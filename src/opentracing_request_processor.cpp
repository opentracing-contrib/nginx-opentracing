#include "opentracing_request_processor.h"
#include <algorithm>
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
  std::copy_n(s.data(), result.len, result.data);
  return result;
}

namespace {
class NgxHeaderCarrierWriter : public lightstep::BasicCarrierWriter {
public:
  explicit NgxHeaderCarrierWriter(ngx_http_request_t *request)
      : request_{request} {}

  void Set(const std::string &key, const std::string &value) const override {
    auto header = reinterpret_cast<ngx_table_elt_t *>(
        ngx_list_push(&request_->headers_in.headers));
    if (!header) {
      // TODO: How do we handle an error?
      std::cerr << "Unable to Set\n";
      return;
    }
    header->hash = 1;
    header->key = ngx_copy_string(request_->pool, key);
    if (!header->key.data) {
      // TODO: How do we handle an error?
      std::cerr << "Unable to Set\n";
      return;
    }
    header->value = ngx_copy_string(request_->pool, value);
    if (!header->value.data) {
      // TODO: How do we handle an error?
      std::cerr << "Unable to Set\n";
      return;
    }
  }

private:
  ngx_http_request_t *request_;
};
}

namespace ngx_opentracing {
static lightstep::Tracer initialize_tracer(const std::string &options) {
  lightstep::TracerOptions tracer_options;
  tracer_options.access_token = options;
  tracer_options.collector_host = "collector-grpc.lightstep.com";
  tracer_options.collector_port = 443;
  tracer_options.collector_encryption = "tls";
  lightstep::BasicRecorderOptions recorder_options;
  return NewLightStepTracer(tracer_options, recorder_options);
}

OpenTracingRequestProcessor::OpenTracingRequestProcessor(
    const std::string &options)
    : tracer_{initialize_tracer(options)} {}

void OpenTracingRequestProcessor::before_response(ngx_http_request_t *request) {
  // TODO: Check to see if we can extract a span from the request headers
  // to use as the parent.
  std::string operation_name{reinterpret_cast<char *>(request->uri.data),
                             request->uri.len};
  auto span = tracer_.StartSpan(operation_name);

  // Inject the context.
  auto carrier_writer = NgxHeaderCarrierWriter{request};
  tracer_.Inject(span.context(), lightstep::CarrierFormat::HTTPHeaders,
                 carrier_writer);

  // Store the span
  active_spans_[request] = std::move(span);
}

void OpenTracingRequestProcessor::after_response(ngx_http_request_t *request) {
  auto span_iter = active_spans_.find(request);
  if (span_iter == std::end(active_spans_)) {
    std::cerr << "Could not find span\n";
    return;
  }
  span_iter->second.Finish();
  active_spans_.erase(span_iter);
}
} // namespace ngx_opentracing
