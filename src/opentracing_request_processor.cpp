#include "opentracing_request_processor.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <lightstep/impl.h>
#include <lightstep/recorder.h>

static ngx_str_t ngx_copy_string(ngx_pool_t *pool, const std::string &s) {
  ngx_str_t result;
  result.data = reinterpret_cast<unsigned char *>(ngx_palloc(pool, s.size()));
  if (!result.data) {
    return {0, nullptr};
  }
  result.len = s.size();
  std::copy(s.begin(), s.end(), result.data);
  return result;
}

static ngx_str_t ngx_copy_key(ngx_pool_t *pool, const std::string &s) {
  ngx_str_t result;
  result.data = reinterpret_cast<unsigned char *>(ngx_palloc(pool, s.size()));
  if (!result.data) {
    return {0, nullptr};
  }
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

static void set_headers(ngx_http_request_t *request,
                        std::vector<std::pair<ngx_str_t, ngx_str_t>> &headers) {
  if (headers.empty())
    return;
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
  for (const auto &key_value : headers) {
    // TODO: Check for errors.
    insert_header(request, key_value.first, key_value.second);
  }
}

namespace {
class NgxHeaderCarrierWriter : public lightstep::BasicCarrierWriter {
public:
  NgxHeaderCarrierWriter(ngx_http_request_t *request,
                         std::vector<std::pair<ngx_str_t, ngx_str_t>> &headers)
      : request_{request}, headers_{headers} {}

  void Set(const std::string &key, const std::string &value) const override {
    // TODO: Check for cases where allocation fails.
    headers_.emplace_back(ngx_copy_key(request_->pool, key),
                          ngx_copy_string(request_->pool, value));
  }

private:
  ngx_http_request_t *request_;
  std::vector<std::pair<ngx_str_t, ngx_str_t>> &headers_;
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
  // We've already started a span for this request, so do nothing.
  if (!was_found.second)
    return;
  auto &span = was_found.first->second;

  // TODO: Check to see if we can extract a span from the request headers
  // to use as the parent.
  auto carrier_reader = NgxHeaderCarrierReader{request};
  auto parent_span_context =
      tracer_.Extract(lightstep::CarrierFormat::HTTPHeaders, carrier_reader);
  std::string operation_name{reinterpret_cast<char *>(request->uri.data),
                             request->uri.len};
  if (parent_span_context.valid()) {
    std::cerr << "starting child span " << operation_name << "...\n";
    span = tracer_.StartSpan(operation_name,
                             {lightstep::ChildOf(parent_span_context)});
  } else {
    std::cerr << "starting span " << operation_name << "...\n";
    span = tracer_.StartSpan(operation_name);
  }
  span.SetTag("component", "nginx");
  span.SetTag("http.method",
              std::string{reinterpret_cast<char *>(request->method_name.data),
                          request->method_name.len});
  span.SetTag("http.uri",
              std::string{reinterpret_cast<char *>(request->unparsed_uri.data),
                          request->unparsed_uri.len});

  // Inject the context.
  std::vector<std::pair<ngx_str_t, ngx_str_t>> headers;
  auto carrier_writer = NgxHeaderCarrierWriter{request, headers};
  if (tracer_.Inject(span.context(), lightstep::CarrierFormat::HTTPHeaders,
                     carrier_writer)) {
    set_headers(request, headers);
  } else {
    std::cerr << "Tracer.inject() failed\n";
  }
}

void OpenTracingRequestProcessor::after_response(ngx_http_request_t *request) {
  // Lookup the span.
  auto span_iter = active_spans_.find(request);
  if (span_iter == std::end(active_spans_)) {
    std::cerr << "Could not find span\n";
    return;
  }
  auto &span = span_iter->second;

  // Check for errors.
  // TODO: Should we also look at request->err_status?
  auto status = uint64_t{request->headers_out.status};
  span.SetTag("http.status_code", status);
  if (status != NGX_HTTP_OK && status != NGX_OK) {
    span.SetTag("error", true);
    // TODO: Log error values in request->headers_out.status_line to span.
  }

  // Finish the span.
  span.Finish();
  active_spans_.erase(span_iter);
}
} // namespace ngx_opentracing
