#pragma once

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
ngx_int_t discover_span_context_keys(ngx_log_t* log,
                                     const char* tracing_library,
                                     const char* tracer_config_file,
                                     ngx_array_t* span_context_keys);
}  // namespace ngx_opentracing
