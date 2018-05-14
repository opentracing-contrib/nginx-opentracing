#include "opentracing_handler.h"

#include "opentracing_request_instrumentor.h"

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// is_opentracing_enabled
//------------------------------------------------------------------------------
static bool is_opentracing_enabled(
    const ngx_http_request_t *request,
    const ngx_http_core_loc_conf_t *core_loc_conf,
    const opentracing_loc_conf_t *loc_conf) noexcept {
  // Check if this is a main request instead of a subrequest.
  if (request == request->main)
    return loc_conf->enable;
  else
    // Only trace subrequests if `log_subrequest` is enabled; otherwise the
    // spans won't be finished.
    return loc_conf->enable && core_loc_conf->log_subrequest;
}

//------------------------------------------------------------------------------
// on_enter_block
//------------------------------------------------------------------------------
ngx_int_t on_enter_block(ngx_http_request_t *request) noexcept try {
  auto core_loc_conf = static_cast<ngx_http_core_loc_conf_t *>(
      ngx_http_get_module_loc_conf(request, ngx_http_core_module));
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(
      ngx_http_get_module_loc_conf(request, ngx_http_opentracing_module));
  if (!is_opentracing_enabled(request, core_loc_conf, loc_conf))
    return NGX_DECLINED;

  auto instrumentor = static_cast<OpenTracingRequestInstrumentor *>(
      ngx_http_get_module_ctx(request, ngx_http_opentracing_module));
  if (instrumentor == nullptr) {
    instrumentor =
        new OpenTracingRequestInstrumentor{request, core_loc_conf, loc_conf};
    ngx_http_set_ctx(request, static_cast<void *>(instrumentor),
                     ngx_http_opentracing_module);
  } else {
    try {
      instrumentor->on_change_block(core_loc_conf, loc_conf);
    } catch (...) {
      delete instrumentor;
      ngx_http_set_ctx(request, nullptr, ngx_http_opentracing_module);
      throw;
    }
  }
  return NGX_DECLINED;
} catch (const std::exception &e) {
  ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                "OpenTracing instrumentation failed for request %p: %s",
                request, e.what());
  return NGX_DECLINED;
}

//------------------------------------------------------------------------------
// on_log_request
//------------------------------------------------------------------------------
ngx_int_t on_log_request(ngx_http_request_t *request) noexcept {
  auto instrumentor = static_cast<OpenTracingRequestInstrumentor *>(
      ngx_http_get_module_ctx(request, ngx_http_opentracing_module));
  if (instrumentor == nullptr) return NGX_DECLINED;
  try {
    instrumentor->on_log_request();
  } catch (const std::exception &e) {
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                  "OpenTracing instrumentation failed for request %p: %s",
                  request, e.what());
  }
  delete instrumentor;
  ngx_http_set_ctx(request, nullptr, ngx_http_opentracing_module);
  return NGX_DECLINED;
}
}  // namespace ngx_opentracing
