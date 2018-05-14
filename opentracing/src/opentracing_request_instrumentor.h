#pragma once

#include "opentracing_conf.h"
#include "span_context_querier.h"

#include <opentracing/tracer.h>
#include <chrono>
#include <memory>

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
class OpenTracingRequestInstrumentor {
 public:
  OpenTracingRequestInstrumentor(ngx_http_request_t *request,
                                 ngx_http_core_loc_conf_t *core_loc_conf,
                                 opentracing_loc_conf_t *loc_conf);

  void on_change_block(ngx_http_core_loc_conf_t *core_loc_conf,
                       opentracing_loc_conf_t *loc_conf);

  void on_log_request();

  ngx_str_t lookup_span_context_value(int value_index);

 private:
  ngx_http_request_t *request_;
  opentracing_main_conf_t *main_conf_;
  opentracing_loc_conf_t *loc_conf_;
  SpanContextQuerier span_context_querier_;
  std::unique_ptr<opentracing::Span> request_span_;
  std::unique_ptr<opentracing::Span> span_;

  opentracing::Span &active_span();

  void on_exit_block(std::chrono::steady_clock::time_point finish_timestamp =
                         std::chrono::steady_clock::now());
};
}  // namespace ngx_opentracing
