#pragma once

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
struct opentracing_main_conf_t {
  ngx_str_t tracer_options;
};

struct opentracing_loc_conf_t {
  ngx_flag_t enable;
  ngx_str_t operation_name;
  ngx_array_t *operation_name_lengths;
  ngx_array_t *operation_name_values;
};
} // namespace ngx_opentracing
