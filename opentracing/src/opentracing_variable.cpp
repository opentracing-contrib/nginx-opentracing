#include "opentracing_variable.h"

#include "opentracing_request_instrumentor.h"
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
// extract_variable_index
//------------------------------------------------------------------------------
static uint32_t extract_variable_index(opentracing::string_view variable_name) {
  static const auto prefix_length =
      std::strlen(OPENTRACING_SPAN_CONTEXT_HEADER_VALUE);
  if (prefix_length <= variable_name.size()) {
    throw std::runtime_error{"invalid variable name length " +
                             std::to_string(variable_name.size())};
  }

  // copy the digit part of the variable so as to ensure it's null-terminated.
  constexpr int max_digits = std::numeric_limits<uint32_t>::digits10;
  char s[max_digits + 1];
  auto num_digits = variable_name.size() - prefix_length;
  if (num_digits > max_digits) {
    throw std::runtime_error{"invalid variable name length " +
                             std::to_string(variable_name.size())};
  }
  std::copy_n(variable_name.data() + prefix_length, num_digits, s);
  s[num_digits] = '\0';

  char* endptr = nullptr;
  errno = 0;
  auto result = std::strtoul(s, &endptr, 10);
  if (errno != 0) {
    throw std::runtime_error{"variable index " + std::string{s} +
                             " is out of range"};
  }

  return static_cast<uint32_t>(result);
}

//------------------------------------------------------------------------------
// expand_span_context_value_variable
//------------------------------------------------------------------------------
// Expands to the ith value of the active span context where i is determined
// from the variable's suffix.
//
// See propagate_opentracing_context
static ngx_int_t expand_span_context_value_variable(
    ngx_http_request_t* request, ngx_http_variable_value_t* variable_value,
    uintptr_t data) noexcept try {
  auto& variable_name = *reinterpret_cast<ngx_str_t*>(data);
  auto variable_index = extract_variable_index(to_string_view(variable_name));

  auto instrumentor = static_cast<OpenTracingRequestInstrumentor*>(
      ngx_http_get_module_ctx(request, ngx_http_opentracing_module));
  if (instrumentor == nullptr) {
    throw std::runtime_error{
        "no OpenTracingRequestInstrumentor attached to request"};
  }

  auto span_context_value =
      instrumentor->lookup_span_context_value(variable_index);

  variable_value->len = span_context_value.len;
  variable_value->valid = 1;
  variable_value->no_cacheable = 1;
  variable_value->not_found = 0;
  variable_value->data = span_context_value.data;

  return NGX_OK;
} catch (const std::exception& e) {
  ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                "failed to expand " OPENTRACING_SPAN_CONTEXT_HEADER_VALUE
                " for request %p: %s",
                request, e.what());
  return NGX_ERROR;
}

//------------------------------------------------------------------------------
// add_variables
//------------------------------------------------------------------------------
ngx_int_t add_variables(ngx_conf_t* cf) noexcept {
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
