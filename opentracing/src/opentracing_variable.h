#pragma once

#include <opentracing/string_view.h>

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <string>
#include <utility>
#include <vector>

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// opentracing_context_variable_name
//------------------------------------------------------------------------------
extern const opentracing::string_view opentracing_context_variable_name;

//------------------------------------------------------------------------------
// add_variables
//------------------------------------------------------------------------------
ngx_int_t add_variables(ngx_conf_t* cf) noexcept;
}  // namespace ngx_opentracing
