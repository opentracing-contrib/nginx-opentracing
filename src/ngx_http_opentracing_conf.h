#pragma once

#include "ngx_script.h"
#include <opentracing_tracer_options.h>

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
  tracer_options_t tracer_options;
  ngx_array_t *tags;
};

struct opentracing_loc_conf_t {
  ngx_flag_t enable;
  NgxScript operation_name_script;
  ngx_array_t *tags;
};
} // namespace ngx_opentracing
