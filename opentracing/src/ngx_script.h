#pragma once

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
class NgxScript {
 public:
  // Note: Constructor is compatible with begin zero initialized.
  NgxScript();

  bool is_valid() const { return pattern_.data; }

  ngx_int_t compile(ngx_conf_t *cf, const ngx_str_t &pattern);
  ngx_str_t run(ngx_http_request_t *request) const;

 private:
  ngx_str_t pattern_;
  ngx_array_t *lengths_;
  ngx_array_t *values_;
};
}  // namespace ngx_opentracing
