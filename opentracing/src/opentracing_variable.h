#pragma once

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <string>
#include <utility>
#include <vector>

#define OPENTRACING_SPAN_CONTEXT_HEADER_VALUE \
  "opentracing_internal_span_context_value"

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// add_variables
//------------------------------------------------------------------------------
ngx_int_t add_variables(ngx_conf_t* cf) noexcept;
}  // namespace ngx_opentracing
