#include "opentracing_handler.h"
#include <opentracing/tracer.h>
#include <unordered_map>
#include "opentracing_conf.h"
#include "opentracing_request_instrumentor.h"

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// OpenTracingContext
//------------------------------------------------------------------------------
namespace {
class OpenTracingContext {
 public:
  void on_enter_block(ngx_http_request_t *request);
  void on_log_request(ngx_http_request_t *request);

 private:
  std::unordered_map<const ngx_http_request_t *, OpenTracingRequestInstrumentor>
      active_instrumentors_;
};
}  // namespace

//------------------------------------------------------------------------------
// get_handler_context
//------------------------------------------------------------------------------
static OpenTracingContext &get_handler_context() {
  static OpenTracingContext handler_context{};
  return handler_context;
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

ngx_int_t on_enter_block(ngx_http_request_t *request) {
  get_handler_context().on_enter_block(request);
  return NGX_DECLINED;
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

ngx_int_t on_log_request(ngx_http_request_t *request) {
  get_handler_context().on_log_request(request);
  return NGX_DECLINED;
}
}  // namespace ngx_opentracing
