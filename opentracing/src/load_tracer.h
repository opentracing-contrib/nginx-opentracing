#pragma once

#include <opentracing/dynamic_load.h>

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
ngx_int_t load_tracer(ngx_log_t* log, const char* tracer_library,
                      const char* config_file,
                      opentracing::DynamicTracingLibraryHandle& handle,
                      std::shared_ptr<opentracing::Tracer>& tracer);
}  // namespace ngx_opentracing
