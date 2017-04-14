#include "request_instrumentation.h"
#include <lightstep/tracer.h>

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
lightstep::SpanContext extract_span_context(lightstep::Tracer& tracer,
                                            const ngx_http_request_t* request);

void inject_span_context(lightstep::Tracer& tracer, ngx_http_request_t* request,
                         const lightstep::SpanContext span_context);
//------------------------------------------------------------------------------
// RequestInstrumentation
//------------------------------------------------------------------------------
RequestInstrumentation::RequestInstrumentation(
    ngx_http_request_t* request, ngx_http_core_loc_conf_t* core_loc_conf,
    opentracing_loc_conf_t* loc_conf)
    : request_{request} {
  main_conf_ = static_cast<opentracing_main_conf_t*>(
      ngx_http_get_module_main_conf(request, ngx_http_opentracing_module));
  (void)request_;
  (void)main_conf_;
}

//------------------------------------------------------------------------------
// on_enter_block
//------------------------------------------------------------------------------
void RequestInstrumentation::on_enter_block(
    ngx_http_core_loc_conf_t* core_loc_conf, opentracing_loc_conf_t* loc_conf) {
}

//------------------------------------------------------------------------------
// on_log_request
//------------------------------------------------------------------------------
void RequestInstrumentation::on_log_request() {}
}  // namespace ngx_opentracing
