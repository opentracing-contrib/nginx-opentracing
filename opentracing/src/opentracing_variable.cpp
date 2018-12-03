#include "opentracing_variable.h"

#include "opentracing_context.h"
#include "utility.h"

#include <opentracing/string_view.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// opentracing_context_variable_name
//------------------------------------------------------------------------------
const opentracing::string_view opentracing_context_variable_name{
    "opentracing_context_"};

//------------------------------------------------------------------------------
// opentracing_binary_context_variable_name
//------------------------------------------------------------------------------
static const opentracing::string_view opentracing_binary_context_variable_name{
    "opentracing_binary_context"};

//------------------------------------------------------------------------------
// expand_opentracing_context
//------------------------------------------------------------------------------
// Extracts the key specified by the variable's suffix and expands to the
// corresponding value of the active span context.
//
// See propagate_opentracing_context
static ngx_int_t expand_opentracing_context_variable(
    ngx_http_request_t* request, ngx_http_variable_value_t* variable_value,
    uintptr_t data) noexcept try {
  auto variable_name = to_string_view(*reinterpret_cast<ngx_str_t*>(data));
  auto prefix_length = opentracing_context_variable_name.size();

  opentracing::string_view key{variable_name.data() + prefix_length,
                               variable_name.size() - prefix_length};

  auto context = get_opentracing_context(request);
  if (context == nullptr) {
    throw std::runtime_error{"no OpenTracingContext attached to request"};
  }

  auto span_context_value = context->lookup_span_context_value(request, key);

  variable_value->len = span_context_value.len;
  variable_value->valid = 1;
  variable_value->no_cacheable = 1;
  variable_value->not_found = 0;
  variable_value->data = span_context_value.data;

  return NGX_OK;
} catch (const std::exception& e) {
  ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                "failed to expand %s"
                " for request %p: %s",
                opentracing_context_variable_name.data(), request, e.what());
  return NGX_ERROR;
}

//------------------------------------------------------------------------------
// expand_opentracing_binary_context_variable
//------------------------------------------------------------------------------
static ngx_int_t expand_opentracing_binary_context_variable(
    ngx_http_request_t* request, ngx_http_variable_value_t* variable_value,
    uintptr_t data) noexcept try {
  auto context = get_opentracing_context(request);
  if (context == nullptr) {
    throw std::runtime_error{"no OpenTracingContext attached to request"};
  }
  auto binary_context = context->get_binary_context(request);
  variable_value->len = binary_context.len;
  variable_value->valid = 1;
  variable_value->no_cacheable = 1;
  variable_value->not_found = 0;
  variable_value->data = binary_context.data;

  return NGX_OK;
} catch (const std::exception& e) {
  ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                "failed to expand %s"
                " for request %p: %s",
                opentracing_context_variable_name.data(), request, e.what());
  return NGX_ERROR;
}

//------------------------------------------------------------------------------
// add_variables
//------------------------------------------------------------------------------
ngx_int_t add_variables(ngx_conf_t* cf) noexcept {
  auto opentracing_context = to_ngx_str(opentracing_context_variable_name);
  auto opentracing_context_var = ngx_http_add_variable(
      cf, &opentracing_context,
      NGX_HTTP_VAR_NOCACHEABLE | NGX_HTTP_VAR_NOHASH | NGX_HTTP_VAR_PREFIX);
  opentracing_context_var->get_handler = expand_opentracing_context_variable;
  opentracing_context_var->data = 0;

  auto opentracing_binary_context =
      to_ngx_str(opentracing_binary_context_variable_name);
  auto opentracing_binary_context_var = ngx_http_add_variable(
      cf, &opentracing_binary_context, NGX_HTTP_VAR_NOCACHEABLE);
  opentracing_binary_context_var->get_handler =
      expand_opentracing_binary_context_variable;
  opentracing_binary_context_var->data = 0;

  return NGX_OK;
}
}  // namespace ngx_opentracing
