#pragma once

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
// opentracing_conf_handler is copied from
//    https://github.com/nginx/nginx/blob/0ad556fe59ad132dc4d34dea9e80f2ff2c3c1314/src/core/ngx_conf_file.c
// this is necessary for OpenTracing's implementation of context propagation.
//
// See http://mailman.nginx.org/pipermail/nginx-devel/2018-March/011008.html
ngx_int_t opentracing_conf_handler(ngx_conf_t *cf, ngx_int_t last) noexcept;
}  // namespace ngx_opentracing
