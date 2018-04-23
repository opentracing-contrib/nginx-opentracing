#include "opentracing_context.h"

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {

//------------------------------------------------------------------------------
// instance
//------------------------------------------------------------------------------
OpenTracingContext &OpenTracingContext::instance() {
  static OpenTracingContext context;
  return context;
}

//------------------------------------------------------------------------------
// is_opentracing_enabled
//------------------------------------------------------------------------------
static bool is_opentracing_enabled(
    const ngx_http_request_t *request,
    const ngx_http_core_loc_conf_t *core_loc_conf,
    const opentracing_loc_conf_t *loc_conf) {
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
void OpenTracingContext::on_enter_block(ngx_http_request_t *request) {
  auto core_loc_conf = static_cast<ngx_http_core_loc_conf_t *>(
      ngx_http_get_module_loc_conf(request, ngx_http_core_module));
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(
      ngx_http_get_module_loc_conf(request, ngx_http_opentracing_module));

  auto instrumentor_iter = active_instrumentors_.find(request);
  if (instrumentor_iter == active_instrumentors_.end()) {
    if (!is_opentracing_enabled(request, core_loc_conf, loc_conf)) return;
    try {
      active_instrumentors_.emplace(
          request,
          OpenTracingRequestInstrumentor{request, core_loc_conf, loc_conf});
    } catch (const InstrumentationFailure &) {
      ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                    "OpenTracing instrumentation failed for request %p",
                    request);
    }
  } else {
    try {
      instrumentor_iter->second.on_change_block(core_loc_conf, loc_conf);
    } catch (const InstrumentationFailure &) {
      active_instrumentors_.erase(instrumentor_iter);
      ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                    "OpenTracing instrumentation failed for request %p",
                    request);
    }
  }
}

//------------------------------------------------------------------------------
// on_log_request
//------------------------------------------------------------------------------
void OpenTracingContext::on_log_request(ngx_http_request_t *request) {
  auto instrumentor_iter = active_instrumentors_.find(request);
  if (instrumentor_iter == active_instrumentors_.end()) return;
  try {
    instrumentor_iter->second.on_log_request();
  } catch (const InstrumentationFailure &) {
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                  "OpenTracing instrumentation failed for request %p", request);
  }
  active_instrumentors_.erase(instrumentor_iter);
}
}  // namespace ngx_opentracing
