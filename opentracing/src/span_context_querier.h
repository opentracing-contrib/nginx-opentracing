#pragma once

#include "opentracing_conf.h"

#include <opentracing/span.h>

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
class SpanContextQuerier {
 public:
  explicit SpanContextQuerier(const opentracing_main_conf_t& conf);

  ngx_str_t lookup_value(ngx_http_request_t* request, const opentracing::Span& span,
                         int value_index);

 private:
  int num_keys_;
  opentracing::string_view* keys_;

  const opentracing::Span* values_span_ = nullptr;
  opentracing::string_view* values_ = nullptr;

  void expand_span_context_values(ngx_http_request_t* request,
                                  const opentracing::Span& span);
};
} // namespace ngx_opentracing
