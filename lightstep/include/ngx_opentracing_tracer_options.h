#pragma once

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
struct tracer_options_t {
  ngx_str_t access_token;
  ngx_str_t component_name;
  ngx_str_t collector_host;
  ngx_str_t collector_encryption;
  ngx_str_t collector_port;
};
}  // namespace ngx_opentracing
