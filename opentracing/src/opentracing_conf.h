#pragma once

#include "ngx_script.h"

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
struct opentracing_tag_t {
  NgxScript key_script;
  NgxScript value_script;
};

struct opentracing_main_conf_t {
  ngx_array_t *tags;
  ngx_str_t tracer_library;
  ngx_str_t tracer_conf_file;
  ngx_array_t *span_context_keys;
};

struct opentracing_loc_conf_t {
  ngx_flag_t enable;
  ngx_flag_t enable_locations;
  NgxScript operation_name_script;
  NgxScript loc_operation_name_script;
  ngx_flag_t trust_incoming_span;
  ngx_array_t *tags;
};
}  // namespace ngx_opentracing
