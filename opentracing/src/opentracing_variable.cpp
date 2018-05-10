#include "opentracing_variable.h"

#include "opentracing_request_instrumentor.h"
#include "utility.h"

#include <opentracing/string_view.h>

#include <iostream>
#include <cstring>
#include <limits>
#include <algorithm>
#include <cstdlib>

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// extract_variable_index
//------------------------------------------------------------------------------
static uint32_t extract_variable_index(opentracing::string_view variable_name) {
  static const auto prefix_length =
      std::strlen(OPENTRACING_SPAN_CONTEXT_HEADER_VALUE);
  if (prefix_length <= variable_name.size()) {
    // TODO: Error
  }
  constexpr int max_digits = std::numeric_limits<uint32_t>::digits10;
  // TODO: what if the string is greater than max digits
  char s[max_digits+1];

  // ensure our number string is null terminated
  auto num_digits = variable_name.size() - prefix_length;
  std::copy_n(variable_name.data() + prefix_length, num_digits, s);
  s[num_digits] = '\0';

  
  char* endptr = nullptr;
  auto result = std::strtoul(s, &endptr, 10);

  // TODO: check for errors

  return static_cast<uint32_t>(result);
}

//------------------------------------------------------------------------------
// expand_span_context_value_variable
//------------------------------------------------------------------------------
static ngx_int_t expand_span_context_value_variable(
    ngx_http_request_t* request, ngx_http_variable_value_t* variable_value,
    uintptr_t data) {
  auto& variable_name = *reinterpret_cast<ngx_str_t*>(data);
  auto variable_index = extract_variable_index(
      opentracing::string_view{(char*)variable_name.data, variable_name.len});

  auto instrumentor = static_cast<OpenTracingRequestInstrumentor *>(
      ngx_http_get_module_ctx(request, ngx_http_opentracing_module));
  // TODO: check for null

  auto span_context_value =
      instrumentor->lookup_span_context_value(variable_index);

  variable_value->len = span_context_value.len;
  variable_value->valid = 1;
  variable_value->no_cacheable = 1;
  variable_value->not_found = 0;
  variable_value->data = span_context_value.data;

  return NGX_OK;
}

//------------------------------------------------------------------------------
// add_variables
//------------------------------------------------------------------------------
ngx_int_t add_variables(ngx_conf_t* cf) {
  ngx_str_t span_context_value =
      ngx_string(OPENTRACING_SPAN_CONTEXT_HEADER_VALUE);
  auto var = ngx_http_add_variable(
      cf, &span_context_value,
      NGX_HTTP_VAR_NOCACHEABLE | NGX_HTTP_VAR_NOHASH | NGX_HTTP_VAR_PREFIX);
  var->get_handler = expand_span_context_value_variable;
  var->data = 0;
  return NGX_OK;
}
}  // namespace ngx_opentracing
