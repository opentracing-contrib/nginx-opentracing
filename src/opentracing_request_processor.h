#pragma once

#include <lightstep/tracer.h>
#include <string>
#include <unordered_map>

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
class OpenTracingRequestProcessor {
public:
  explicit OpenTracingRequestProcessor(const std::string &options);

  void before_response(ngx_http_request_t *request);
  void after_response(ngx_http_request_t *request);

private:
  lightstep::Tracer tracer_;
  std::unordered_map<const ngx_http_request_t *, lightstep::Span> active_spans_;
};
} // namespace ngx_opentracing
