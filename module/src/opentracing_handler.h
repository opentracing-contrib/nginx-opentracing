#pragma once

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
ngx_int_t on_enter_block(ngx_http_request_t *request);
ngx_int_t on_log_request(ngx_http_request_t *request);
}  // namespace ngx_opentracing
