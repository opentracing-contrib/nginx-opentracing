#pragma once

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
struct tracer_options_t {
  ngx_str_t collector_host;
  ngx_str_t collector_port;
  ngx_str_t service_name;
};
}  // namespace ngx_opentracing
