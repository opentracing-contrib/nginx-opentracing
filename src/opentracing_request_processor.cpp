#include "opentracing_request_processor.h"
#include <iostream>
#include <lightstep/impl.h>
#include <lightstep/recorder.h>

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
  std::string operation_name{reinterpret_cast<char *>(request->uri.data),
                             request->uri.len};
  auto span = tracer_.StartSpan(operation_name);
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
