#include "opentracing_variable.h"
#include <iostream>
#include "utility.h"

namespace ngx_opentracing {
const int max_context_propagation_headers = 10;

//------------------------------------------------------------------------------
// get_context_propagation_header_variable_names
//------------------------------------------------------------------------------
const std::vector<std::pair<std::string, std::string>>&
get_context_propagation_header_variable_names() {
  static const auto result = [] {
    std::vector<std::pair<std::string, std::string>> header_variables;
    header_variables.reserve(max_context_propagation_headers);
    for (int i = 0; i < max_context_propagation_headers; ++i) {
      std::string key = "opentracing_internal_span_context_key";
      std::string value = "opentracing_internal_span_context_value";
      const auto index_str = std::to_string(i);
      key.append(index_str);
      value.append(index_str);
      header_variables.emplace_back(std::move(key), std::move(value));
    }
    return header_variables;
  }();
  return result;
}

//------------------------------------------------------------------------------
// expand_span_context_key_variable
//------------------------------------------------------------------------------
static ngx_int_t expand_span_context_key_variable(
    ngx_http_request_t* request, ngx_http_variable_value_t* variable_value,
    uintptr_t data) {
  auto s =
      to_ngx_str(request->pool, "ot-span-context-key-" + std::to_string(data));
  variable_value->len = s.len;
  variable_value->valid = 1;
  variable_value->no_cacheable = 1;
  variable_value->not_found = 0;
  variable_value->data = s.data;
  return NGX_OK;
}

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
// add_context_propagation_variables
//------------------------------------------------------------------------------
static ngx_int_t add_context_propagation_variables(ngx_conf_t* cf) {
  uintptr_t index = 0;
  for (auto& name : get_context_propagation_header_variable_names()) {
    ngx_str_t key_name{
        name.first.size(),
        reinterpret_cast<unsigned char*>(const_cast<char*>(name.first.data()))};
    ngx_str_t value_name{name.second.size(),
                         reinterpret_cast<unsigned char*>(
                             const_cast<char*>(name.second.data()))};

    auto key_var = ngx_http_add_variable(
        cf, &key_name, NGX_HTTP_VAR_NOCACHEABLE | NGX_HTTP_VAR_NOHASH);
    if (key_var == nullptr) {
      return NGX_ERROR;
    }
    key_var->get_handler = expand_span_context_key_variable;
    key_var->data = index;

    auto value_var = ngx_http_add_variable(
        cf, &value_name, NGX_HTTP_VAR_NOCACHEABLE | NGX_HTTP_VAR_NOHASH);
    if (value_var == nullptr) {
      return NGX_ERROR;
    }
    value_var->get_handler = expand_span_context_value_variable;
    value_var->data = index;

    ++index;
  }

  return NGX_OK;
}

//------------------------------------------------------------------------------
// add_variables
//------------------------------------------------------------------------------
ngx_int_t add_variables(ngx_conf_t* cf) {
  return add_context_propagation_variables(cf);
}
}  // namespace ngx_opentracing
