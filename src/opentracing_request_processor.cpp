#include "opentracing_request_processor.h"
#include "ngx_http_opentracing_conf.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <lightstep/impl.h>
#include <lightstep/recorder.h>
#include <ngx_opentracing_utility.h>

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
lightstep::Tracer make_tracer(const tracer_options_t &);

static bool
is_opentracing_enabled(const ngx_http_request_t *request,
                       const ngx_http_core_loc_conf_t *core_loc_conf,
                       const opentracing_loc_conf_t *loc_conf) {
  if (request == request->main)
    return loc_conf->enable;
  else
    // Only trace subrequests if `log_subrequest` is enabled; otherwise the
    // spans won't be finished.
    return loc_conf->enable && core_loc_conf->log_subrequest;
}

static bool insert_header(ngx_http_request_t *request, ngx_str_t key,
                          ngx_str_t value) {
  auto header = static_cast<ngx_table_elt_t *>(
      ngx_list_push(&request->headers_in.headers));
  if (!header)
    return false;
  header->hash = 1;
  header->key = key;
  header->lowcase_key = key.data;
  header->value = value;
  return true;
}

static bool set_headers(ngx_http_request_t *request,
                        std::vector<std::pair<ngx_str_t, ngx_str_t>> &headers) {
  if (headers.empty())
    return true;

  // If header keys are already in the request, overwrite the values instead of
  // inserting a new header.
  //
  // It may be possible in some cases to use nginx's hashes to look up the
  // entries faster, but then we'd have to handle the special case of when a
  // header element isn't hashed yet. Iterating over the header entries all the
  // time keeps things simple.
  for_each<ngx_table_elt_t>(
      request->headers_in.headers, [&](ngx_table_elt_t &header) {
        auto i = std::find_if(
            headers.begin(), headers.end(),
            [&](const std::pair<ngx_str_t, ngx_str_t> &key_value) {
              const auto &key = key_value.first;
              return header.key.len == key.len &&
                     ngx_strncmp(reinterpret_cast<char *>(header.lowcase_key),
                                 reinterpret_cast<char *>(key.data),
                                 key.len) == 0;

            });
        if (i == headers.end())
          return;
        header.value = i->second;
        headers.erase(i);
      });

  // Any header left in `headers` doesn't already have a key in the request, so
  // create a new entry for it.
  for (const auto &key_value : headers) {
    if (!insert_header(request, key_value.first, key_value.second)) {
      ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                    "failed to insert header");
      return false;
    }
  }
  return true;
}

namespace {
class NgxHeaderCarrierWriter : public lightstep::BasicCarrierWriter {
public:
  NgxHeaderCarrierWriter(ngx_http_request_t *request,
                         std::vector<std::pair<ngx_str_t, ngx_str_t>> &headers,
                         bool &was_successful)
      : request_{request}, headers_{headers}, was_successful_{was_successful} {
    was_successful_ = true;
  }

  void Set(const std::string &key, const std::string &value) const override {
    if (!was_successful_)
      return;
    auto ngx_key = to_lower_ngx_str(request_->pool, key);
    if (!ngx_key.data) {
      ngx_log_error(NGX_LOG_ERR, request_->connection->log, 0,
                    "failed to allocate header key");
      was_successful_ = false;
      return;
    }
    auto ngx_value = to_ngx_str(request_->pool, value);
    if (!ngx_value.data) {
      ngx_log_error(NGX_LOG_ERR, request_->connection->log, 0,
                    "failed to allocate header value");
      was_successful_ = false;
      return;
    }
    headers_.emplace_back(ngx_key, ngx_value);
  }

private:
  ngx_http_request_t *request_;
  std::vector<std::pair<ngx_str_t, ngx_str_t>> &headers_;
  bool &was_successful_;
};
}

namespace {
class NgxHeaderCarrierReader : public lightstep::BasicCarrierReader {
public:
  explicit NgxHeaderCarrierReader(const ngx_http_request_t *request)
      : request_{request} {}

  void ForeachKey(
      std::function<void(const std::string &, const std::string &value)> f)
      const {
    std::string key, value;
    for_each<ngx_table_elt_t>(
        request_->headers_in.headers, [&](const ngx_table_elt_t &header) {
          key.assign(reinterpret_cast<char *>(header.lowcase_key),
                     header.key.len);
          value.assign(reinterpret_cast<char *>(header.value.data),
                       header.value.len);
          f(key, value);
        });
  }

private:
  const ngx_http_request_t *request_;
};
}

static void add_script_tag(ngx_http_request_t *request, lightstep::Span &span,
                           const opentracing_tag_t &tag) {
  auto key = tag.key_script.run(request);
  auto value = tag.value_script.run(request);
  if (key.data && value.data)
    span.SetTag(to_string(key), to_string(value));
}

static lightstep::Span
start_span(ngx_http_request_t *request,
           const ngx_http_core_loc_conf_t *core_loc_conf,
           const opentracing_loc_conf_t *loc_conf, lightstep::Tracer &tracer,
           const lightstep::SpanContext &reference_span_context,
           lightstep::SpanReferenceType reference_type) {
  auto main_conf = static_cast<opentracing_main_conf_t *>(
      ngx_http_get_module_main_conf(request, ngx_http_opentracing_module));

  // Start a new span for the location block.
  std::string operation_name;
  if (loc_conf->operation_name_script.is_valid())
    operation_name = to_string(loc_conf->operation_name_script.run(request));
  else
    operation_name = to_string(core_loc_conf->name);
  lightstep::Span span;
  if (reference_span_context.valid()) {
    std::cerr << "starting child span " << operation_name << "...\n";
    span = tracer.StartSpan(
        operation_name,
        {lightstep::SpanReference{reference_type, reference_span_context}});
  } else {
    std::cerr << "starting span " << operation_name << "...\n";
    span = tracer.StartSpan(operation_name);
  }

  // Set default span tags.
  if (main_conf->tags)
    for_each<opentracing_tag_t>(*main_conf->tags,
                                [&](const opentracing_tag_t &tag) {
                                  add_script_tag(request, span, tag);
                                });

  // Set custom span tags.
  if (loc_conf->tags)
    for_each<opentracing_tag_t>(*loc_conf->tags,
                                [&](const opentracing_tag_t &tag) {
                                  add_script_tag(request, span, tag);
                                });

  // Inject the span's context into the request headers.
  std::vector<std::pair<ngx_str_t, ngx_str_t>> headers;
  bool was_successful = true;
  auto carrier_writer =
      NgxHeaderCarrierWriter{request, headers, was_successful};
  was_successful =
      tracer.Inject(span.context(), lightstep::CarrierFormat::HTTPHeaders,
                    carrier_writer) &&
      was_successful;
  if (was_successful)
    was_successful = set_headers(request, headers);
  if (!was_successful)
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                  "Tracer.inject() failed");

  return span;
}

OpenTracingRequestProcessor::OpenTracingRequestProcessor(
    const tracer_options_t &options)
    : tracer_{make_tracer(options)} {}

void OpenTracingRequestProcessor::before_response(ngx_http_request_t *request) {
  auto core_loc_conf = static_cast<ngx_http_core_loc_conf_t *>(
      ngx_http_get_module_loc_conf(request, ngx_http_core_module));
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(
      ngx_http_get_module_loc_conf(request, ngx_http_opentracing_module));

  auto span_iter = active_spans_.find(request);
  if (span_iter == active_spans_.end()) {
    if (!is_opentracing_enabled(request, core_loc_conf, loc_conf))
      return;
    // No span has been created for this request yet. Check if there's any
    // parent span context in the request headers and start a new one.
    auto carrier_reader = NgxHeaderCarrierReader{request};
    auto parent_span_context =
        tracer_.Extract(lightstep::CarrierFormat::HTTPHeaders, carrier_reader);
    auto span = start_span(request, core_loc_conf, loc_conf, tracer_,
                           parent_span_context, lightstep::ChildOfRef);
    active_spans_.emplace(request, std::move(span));
  } else {
    // A span's already been created for the request, but nginx is entering a
    // new location block. Finish the span for the previous location block and
    // create a new span that follows from it.
    auto &span = span_iter->second;
    span.Finish();
    span = start_span(request, core_loc_conf, loc_conf, tracer_, span.context(),
                      lightstep::FollowsFromRef);
  }
}

void OpenTracingRequestProcessor::after_response(ngx_http_request_t *request) {
  // Lookup the span.
  auto span_iter = active_spans_.find(request);
  if (span_iter == std::end(active_spans_))
    return;

  auto &span = span_iter->second;

  // Check for errors.
  // TODO: Should we also look at request->err_status?
  auto status = uint64_t{request->headers_out.status};
  const auto &status_line = request->headers_out.status_line;
  span.SetTag("http.status_code", status);
  if (status_line.data)
    span.SetTag("http.status_line", to_string(status_line));

  // Treat any 4xx and 5xx code as an error.
  if (status >= 400) {
    span.SetTag("error", true);
    // TODO: Log error values in request->headers_out.status_line to span.
  }

  // Finish the span.
  span.Finish();
  active_spans_.erase(span_iter);
}
} // namespace ngx_opentracing
