#include "opentracing_handler.h"
#include "ngx_http_opentracing_conf.h"
#include "request_instrumentation.h"
#include <lightstep/tracer.h>

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
lightstep::Tracer make_tracer(const tracer_options_t & options);

//------------------------------------------------------------------------------
// make_tracer
//------------------------------------------------------------------------------
static lightstep::Tracer make_tracer(const ngx_http_request_t* request) {
  auto main_conf = static_cast<opentracing_main_conf_t*>(
      ngx_http_get_module_main_conf(request, ngx_http_opentracing_module));
  return make_tracer(main_conf->tracer_options);
}

//------------------------------------------------------------------------------
// OpenTracingContext
//------------------------------------------------------------------------------
namespace {
class OpenTracingContext {
 public:
  explicit OpenTracingContext(const ngx_http_request_t* request) {
    lightstep::Tracer::InitGlobal(make_tracer(request));
  }

  void on_enter_block(ngx_http_request_t* request);
  void on_log_request(ngx_http_request_t* request);

 private:
  std::unordered_map<const ngx_http_request_t*, RequestInstrumentation>
      active_instrumentations_;
};
}

//------------------------------------------------------------------------------
// get_handler_context
//------------------------------------------------------------------------------
static OpenTracingContext& get_handler_context(const ngx_http_request_t* request) {
  static OpenTracingContext handler_context{request};
  return handler_context;
}

//------------------------------------------------------------------------------
// is_opentracing_enabled
//------------------------------------------------------------------------------
static bool
is_opentracing_enabled(const ngx_http_request_t *request,
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
void OpenTracingContext::on_enter_block(ngx_http_request_t* request) {
  auto core_loc_conf = static_cast<ngx_http_core_loc_conf_t*>(
      ngx_http_get_module_loc_conf(request, ngx_http_core_module));
  auto loc_conf = static_cast<opentracing_loc_conf_t*>(
      ngx_http_get_module_loc_conf(request, ngx_http_opentracing_module));

  auto instrumentation_iter = active_instrumentations_.find(request);
  if (instrumentation_iter == active_instrumentations_.end()) {
    if (!is_opentracing_enabled(request, core_loc_conf, loc_conf)) return;
    active_instrumentations_.emplace(
        request, RequestInstrumentation{request, core_loc_conf, loc_conf});
  } else {
    instrumentation_iter->second.on_enter_block(core_loc_conf, loc_conf);
  }
}

ngx_int_t on_enter_block(ngx_http_request_t *request) {
  get_handler_context(request).on_enter_block(request);
  return NGX_DECLINED;
}

//------------------------------------------------------------------------------
// on_log_request
//------------------------------------------------------------------------------
void OpenTracingContext::on_log_request(ngx_http_request_t* request) {
  auto instrumentation_iter = active_instrumentations_.find(request);
  if (instrumentation_iter == active_instrumentations_.end())
    return;
  instrumentation_iter->second.on_log_request();
  active_instrumentations_.erase(instrumentation_iter);
}

ngx_int_t on_log_request(ngx_http_request_t *request) {
  get_handler_context(request).on_log_request(request);
  return NGX_DECLINED;
}
} // namespace ngx_opentracing
