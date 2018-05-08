#include "opentracing_variable.h"
#include <iostream>
#include "utility.h"

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// expand_span_context_value_variable
//------------------------------------------------------------------------------
static ngx_int_t expand_span_context_value_variable(
    ngx_http_request_t* request, ngx_http_variable_value_t* variable_value,
    uintptr_t data) {
  auto s = to_ngx_str(request->pool,
                      "ot-span-context-value-" + std::to_string(data));
  variable_value->len = s.len;
  variable_value->valid = 1;
  variable_value->no_cacheable = 1;
  variable_value->not_found = 0;
  variable_value->data = s.data;
  return NGX_OK;
}

//------------------------------------------------------------------------------
// add_variables
//------------------------------------------------------------------------------
ngx_int_t add_variables(ngx_conf_t* cf) {
  ngx_str_t span_context_value =
      ngx_string("opentracing_internal_span_context_value");
  auto var = ngx_http_add_variable(
      cf, &span_context_value,
      NGX_HTTP_VAR_NOCACHEABLE | NGX_HTTP_VAR_NOHASH | NGX_HTTP_VAR_PREFIX);
  var->get_handler = expand_span_context_value_variable;
  var->data = 0;
  return NGX_OK;
}
}  // namespace ngx_opentracing
