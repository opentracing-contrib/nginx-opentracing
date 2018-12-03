#include "opentracing_handler.h"

#include "opentracing_context.h"

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

  auto context = get_opentracing_context(request);
  if (context == nullptr) {
    context = new OpenTracingContext{request, core_loc_conf, loc_conf};
    set_opentracing_context(request, context);
  } else {
    try {
      context->on_change_block(request, core_loc_conf, loc_conf);
    } catch (...) {
      // The OpenTracingContext may be broken, destroy it so that we don't
      // attempt to continue tracing.
      destroy_opentracing_context(request);

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
  auto context = get_opentracing_context(request);
  if (context == nullptr) return NGX_DECLINED;
  try {
    context->on_log_request(request);
  } catch (const std::exception &e) {
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                  "OpenTracing instrumentation failed for request %p: %s",
                  request, e.what());
  }
  return NGX_DECLINED;
}
}  // namespace ngx_opentracing
