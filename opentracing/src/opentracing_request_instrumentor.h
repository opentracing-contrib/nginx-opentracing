#pragma once

#include <opentracing/tracer.h>
#include <chrono>
#include <exception>
#include <memory>
#include "opentracing_conf.h"

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
struct InstrumentationFailure : std::exception {
  InstrumentationFailure() = default;
  const char *what() const noexcept override {
    return "InstrumentationFailure";
  }
};

class OpenTracingRequestInstrumentor {
 public:
  OpenTracingRequestInstrumentor(ngx_http_request_t *request,
                                 ngx_http_core_loc_conf_t *core_loc_conf,
                                 opentracing_loc_conf_t *loc_conf);

  void on_change_block(ngx_http_core_loc_conf_t *core_loc_conf,
                       opentracing_loc_conf_t *loc_conf);

  void on_log_request();

  ngx_str_t consume_active_span_context_key();

  ngx_str_t consume_active_span_context_value();

 private:
  ngx_http_request_t *request_;
  opentracing_main_conf_t *main_conf_;
  opentracing_loc_conf_t *loc_conf_;
  std::unique_ptr<opentracing::Span> request_span_;
  std::unique_ptr<opentracing::Span> span_;

  opentracing::Span &active_span();

  void on_exit_block(std::chrono::steady_clock::time_point finish_timestamp =
                         std::chrono::steady_clock::now());
};
}  // namespace ngx_opentracing
