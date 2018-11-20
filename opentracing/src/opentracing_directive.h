#pragma once

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// propagate_opentracing_context
//------------------------------------------------------------------------------
char *propagate_opentracing_context(ngx_conf_t *cf, ngx_command_t *command,
                                    void *conf) noexcept;

//------------------------------------------------------------------------------
// propagate_fastcgi_opentracing_context
//------------------------------------------------------------------------------
char *propagate_fastcgi_opentracing_context(ngx_conf_t *cf,
                                            ngx_command_t *command,
                                            void *conf) noexcept;

//------------------------------------------------------------------------------
// propagate_grpc_opentracing_context
//------------------------------------------------------------------------------
char *propagate_grpc_opentracing_context(ngx_conf_t *cf, ngx_command_t *command,
                                         void *conf) noexcept;

//------------------------------------------------------------------------------
// add_opentracing_tag
//------------------------------------------------------------------------------
char *add_opentracing_tag(ngx_conf_t *cf, ngx_array_t *tags, ngx_str_t key,
                          ngx_str_t value) noexcept;

//------------------------------------------------------------------------------
// set_opentracing_tag
//------------------------------------------------------------------------------
char *set_opentracing_tag(ngx_conf_t *cf, ngx_command_t *command,
                          void *conf) noexcept;

//------------------------------------------------------------------------------
// set_opentracing_operation_name
//------------------------------------------------------------------------------
char *set_opentracing_operation_name(ngx_conf_t *cf, ngx_command_t *command,
                                     void *conf) noexcept;

//------------------------------------------------------------------------------
// set_opentracing_location_operation_name
//------------------------------------------------------------------------------
char *set_opentracing_location_operation_name(ngx_conf_t *cf,
                                              ngx_command_t *command,
                                              void *conf) noexcept;

//------------------------------------------------------------------------------
// set_tracer
//------------------------------------------------------------------------------
char *set_tracer(ngx_conf_t *cf, ngx_command_t *command, void *conf) noexcept;
}  // namespace ngx_opentracing
