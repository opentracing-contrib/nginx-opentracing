#pragma once

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
struct NgxScript {
  bool is_valid() const noexcept { return pattern_.data; }

  ngx_int_t compile(ngx_conf_t *cf, const ngx_str_t &pattern) noexcept;
  ngx_str_t run(ngx_http_request_t *request) const noexcept;

  ngx_str_t pattern_;
  ngx_array_t *lengths_;
  ngx_array_t *values_;
};
}  // namespace ngx_opentracing
