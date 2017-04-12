#pragma once

#include <opentracing_tracer_options.h>

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
struct opentracing_main_conf_t {
  tracer_options_t tracer_options;
};

struct opentracing_tag_t {
  ngx_array_t *key_lengths;
  ngx_array_t *key_values;
  ngx_array_t *value_lengths;
  ngx_array_t *value_values;
};

struct opentracing_loc_conf_t {
  ngx_flag_t enable;
  ngx_str_t operation_name;
  ngx_array_t *operation_name_lengths;
  ngx_array_t *operation_name_values;
  ngx_array_t *tags;
};
} // namespace ngx_opentracing
