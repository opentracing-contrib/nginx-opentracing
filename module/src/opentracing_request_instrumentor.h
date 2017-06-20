#pragma once

#include <opentracing/tracer.h>
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

 private:
  ngx_http_request_t *request_;
  opentracing_main_conf_t *main_conf_;
  opentracing_loc_conf_t *loc_conf_;
  std::unique_ptr<opentracing::Span> request_span_;
  std::unique_ptr<opentracing::Span> span_;

  void on_exit_block();
  void set_request_span_context();
};
}  // namespace ngx_opentracing
