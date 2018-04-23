#pragma once

#include "opentracing_request_instrumentor.h"

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// OpenTracingContext
//------------------------------------------------------------------------------
class OpenTracingContext {
 public:
  void on_enter_block(ngx_http_request_t *request);
  void on_log_request(ngx_http_request_t *request);

  static OpenTracingContext &instance();

 private:
  OpenTracingContext() = default;

  std::unordered_map<const ngx_http_request_t *, OpenTracingRequestInstrumentor>
      active_instrumentors_;
};
}  // namespace ngx_opentracing
