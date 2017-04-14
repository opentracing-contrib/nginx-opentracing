#pragma once
#include "ngx_http_opentracing_conf.h"

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
class RequestInstrumentation {
 public:
  RequestInstrumentation(ngx_http_request_t* request,
                         ngx_http_core_loc_conf_t* core_loc_conf,
                         opentracing_loc_conf_t* loc_conf);

  void on_enter_block(ngx_http_core_loc_conf_t* core_loc_conf,
                      opentracing_loc_conf_t* loc_conf);

  void on_log_request();

 private:
  ngx_http_request_t* request_;
  opentracing_main_conf_t* main_conf_;
};
}  // namespace ngx_opentracing
