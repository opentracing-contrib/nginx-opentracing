#include "opentracing_request_processor.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <lightstep/impl.h>
#include <lightstep/recorder.h>

static ngx_str_t ngx_copy_string(ngx_pool_t *pool, const std::string &s) {
  ngx_str_t result;
  result.data = reinterpret_cast<unsigned char *>(ngx_palloc(pool, s.size()));
  if (!result.data)
    return {0, nullptr};
  result.len = s.size();
  std::copy(s.begin(), s.end(), result.data);
  return result;
}

static ngx_str_t ngx_copy_key(ngx_pool_t *pool, const std::string &s) {
  ngx_str_t result;
  result.data = reinterpret_cast<unsigned char *>(ngx_palloc(pool, s.size()));
  if (!result.data)
    return {0, nullptr};
  result.len = s.size();
  std::transform(s.begin(), s.end(), result.data,
                 [](char c) { return std::tolower(c); });
  return result;
}

static bool insert_header(ngx_http_request_t *request, ngx_str_t key,
                          ngx_str_t value) {
  auto header = reinterpret_cast<ngx_table_elt_t *>(
      ngx_list_push(&request->headers_in.headers));
  if (!header)
    return false;
  header->hash = 1;
  header->key = key;
  header->lowcase_key = key.data;
  header->value = value;
  return true;
}

template <class F>
static void for_each_header(const ngx_http_request_t *request, F f) {
  auto headers_part = &request->headers_in.headers.part;
  auto headers = reinterpret_cast<ngx_table_elt_t *>(headers_part->elts);
  for (ngx_uint_t i = 0;; i++) {
    if (i >= headers_part->nelts) {
      if (!headers_part->next)
        return;
      headers_part = headers_part->next;
      headers = reinterpret_cast<ngx_table_elt_t *>(headers_part->elts);
      i = 0;
    }
    auto &header = headers[i];
    f(header);
  }
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
  for_each_header(request, [&](ngx_table_elt_t &header) {
    auto i = std::find_if(
        headers.begin(), headers.end(),
        [&](const std::pair<ngx_str_t, ngx_str_t> &key_value) {
          const auto &key = key_value.first;
          return header.key.len == key.len &&
                 ngx_strncmp(reinterpret_cast<char *>(header.lowcase_key),
                             reinterpret_cast<char *>(key.data), key.len) == 0;

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
    auto ngx_key = ngx_copy_key(request_->pool, key);
    if (!ngx_key.data) {
      ngx_log_error(NGX_LOG_ERR, request_->connection->log, 0,
                    "failed to allocate header key");
      was_successful_ = false;
      return;
    }
    auto ngx_value = ngx_copy_string(request_->pool, value);
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
    for_each_header(request_, [&](const ngx_table_elt_t &header) {
      key.assign(reinterpret_cast<char *>(header.lowcase_key), header.key.len);
      value.assign(reinterpret_cast<char *>(header.value.data),
                   header.value.len);
      f(key, value);
    });
  }

private:
  const ngx_http_request_t *request_;
};
}

static lightstep::Span
start_location_block_span(ngx_http_request_t *request,
                          lightstep::Tracer &tracer,
                          const lightstep::SpanContext &reference_span_context,
                          lightstep::SpanReferenceType reference_type) {
  // Start a new span for the location block.
  auto core_loc_conf = reinterpret_cast<ngx_http_core_loc_conf_t *>(
      ngx_http_get_module_loc_conf(request, ngx_http_core_module));
  std::string operation_name{reinterpret_cast<char *>(core_loc_conf->name.data),
                             core_loc_conf->name.len};
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
  span.SetTag("component", "nginx");
  span.SetTag("http.method",
              std::string{reinterpret_cast<char *>(request->method_name.data),
                          request->method_name.len});
  span.SetTag("http.uri",
              std::string{reinterpret_cast<char *>(request->unparsed_uri.data),
                          request->unparsed_uri.len});

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

namespace ngx_opentracing {
static lightstep::Tracer initialize_tracer(const std::string &options) {
  // TODO: Will need a way to start an arbitrary tracer here, but to get started
  // just hard-code to the LightStep tracer. Also, `options` may encode multiple
  // settings; for now assume it's the access token.
  lightstep::TracerOptions tracer_options;
  tracer_options.access_token = options;
  tracer_options.collector_host = "collector-grpc.lightstep.com";
  tracer_options.collector_port = 443;
  tracer_options.collector_encryption = "tls";
  tracer_options.tracer_attributes["lightstep.component_name"] = "nginx";
  lightstep::BasicRecorderOptions recorder_options;
  return NewLightStepTracer(tracer_options, recorder_options);
}

OpenTracingRequestProcessor::OpenTracingRequestProcessor(
    const std::string &options)
    : tracer_{initialize_tracer(options)} {}

void OpenTracingRequestProcessor::before_response(ngx_http_request_t *request) {
  auto was_found = active_spans_.emplace(request, lightstep::Span{});
  auto &span = was_found.first->second;
  if (was_found.second) {
    // No span has been created for this request yet. Check if there's any
    // parent span context in the request headers and start a new one.
    auto carrier_reader = NgxHeaderCarrierReader{request};
    auto parent_span_context =
        tracer_.Extract(lightstep::CarrierFormat::HTTPHeaders, carrier_reader);
    span = start_location_block_span(request, tracer_, parent_span_context,
                                     lightstep::ChildOfRef);
  } else {
    // A span's already been created for the request, but nginx is entering a
    // new location block. Finish the span for the previous location block and
    // create a new span that follows from it.
    span.Finish();
    span = start_location_block_span(request, tracer_, span.context(),
                                     lightstep::FollowsFromRef);
  }
}

void OpenTracingRequestProcessor::after_response(ngx_http_request_t *request) {
  // Lookup the span.
  auto span_iter = active_spans_.find(request);
  if (span_iter == std::end(active_spans_)) {
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                  "failed to find associated span for request");
    return;
  }
  auto &span = span_iter->second;

  // Check for errors.
  // TODO: Should we also look at request->err_status?
  auto status = uint64_t{request->headers_out.status};
  span.SetTag("http.status_code", status);
  // TODO: Is it correct to mark the error tag when the status isn't
  // NGX_HTTP_OK? We'd expect certain error statuses to be returned as part of
  // normal operations.
  if (status != NGX_HTTP_OK) {
    span.SetTag("error", true);
    // TODO: Log error values in request->headers_out.status_line to span.
  }

  // Finish the span.
  span.Finish();
  active_spans_.erase(span_iter);
}
} // namespace ngx_opentracing
