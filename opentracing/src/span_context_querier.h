#pragma once

#include "opentracing_conf.h"

#include <opentracing/span.h>

#include <utility>
#include <vector>

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
class SpanContextQuerier {
 public:
  SpanContextQuerier() noexcept {}

  ngx_str_t lookup_value(ngx_http_request_t* request,
                         const opentracing::Span& span,
                         opentracing::string_view key);

 private:
  const opentracing::Span* values_span_ = nullptr;

  std::vector<std::pair<std::string, std::string>> span_context_expansion_;

  void expand_span_context_values(ngx_http_request_t* request,
                                  const opentracing::Span& span);
};
}  // namespace ngx_opentracing
