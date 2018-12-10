#pragma once

#include "opentracing_conf.h"
#include "request_tracing.h"
#include "span_context_querier.h"

#include <opentracing/tracer.h>
#include <chrono>
#include <memory>
#include <vector>

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// OpenTracingContext
//------------------------------------------------------------------------------
class OpenTracingContext {
 public:
  OpenTracingContext(ngx_http_request_t* request,
                     ngx_http_core_loc_conf_t* core_loc_conf,
                     opentracing_loc_conf_t* loc_conf);

  void on_change_block(ngx_http_request_t* request,
                       ngx_http_core_loc_conf_t* core_loc_conf,
                       opentracing_loc_conf_t* loc_conf);

  void on_log_request(ngx_http_request_t* request);

  ngx_str_t lookup_span_context_value(ngx_http_request_t* request,
                                      opentracing::string_view key);

  ngx_str_t get_binary_context(ngx_http_request_t* request) const;

 private:
  std::vector<RequestTracing> traces_;

  RequestTracing* find_trace(ngx_http_request_t* request);

  const RequestTracing* find_trace(ngx_http_request_t* request) const;
};

//------------------------------------------------------------------------------
// get_opentracing_context
//------------------------------------------------------------------------------
OpenTracingContext* get_opentracing_context(
    ngx_http_request_t* request) noexcept;

//------------------------------------------------------------------------------
// set_opentracing_context
//------------------------------------------------------------------------------
void set_opentracing_context(ngx_http_request_t* request,
                             OpenTracingContext* context);

//------------------------------------------------------------------------------
// destroy_opentracing_context
//------------------------------------------------------------------------------
void destroy_opentracing_context(ngx_http_request_t* request) noexcept;
}  // namespace ngx_opentracing
