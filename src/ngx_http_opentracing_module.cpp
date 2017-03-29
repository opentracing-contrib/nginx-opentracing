#include "opentracing_request_processor.h"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <lightstep/impl.h>
#include <lightstep/recorder.h>
#include <lightstep/tracer.h>
#include <unordered_map>

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

extern ngx_module_t ngx_http_opentracing_module;
}

using ngx_opentracing::OpenTracingRequestProcessor;

namespace {
struct opentracing_main_conf_t {
  ngx_str_t tracer_options;
};
}

static OpenTracingRequestProcessor &
get_opentracing_request_processor(ngx_http_request_t *request) {
  static auto request_processor = [request] {
    auto conf = reinterpret_cast<opentracing_main_conf_t *>(
        ngx_http_get_module_main_conf(request, ngx_http_opentracing_module));
    return OpenTracingRequestProcessor{
        std::string{reinterpret_cast<char *>(conf->tracer_options.data),
                    conf->tracer_options.len}};
  }();
  return request_processor;
}

static ngx_int_t before_response_handler(ngx_http_request_t *request) {
  if (request->main->internal)
    return NGX_DECLINED;
  request->main->internal = 1;

  get_opentracing_request_processor(request).before_response(request);

  return NGX_DECLINED;
}

static ngx_int_t after_response_handler(ngx_http_request_t *request) {
  get_opentracing_request_processor(request).after_response(request);

  return NGX_DECLINED;
}

static ngx_int_t ngx_http_opentracing_init(ngx_conf_t *config) {
  auto core_main_config = reinterpret_cast<ngx_http_core_main_conf_t *>(
      ngx_http_conf_get_module_main_conf(config, ngx_http_core_module));

  auto handler = reinterpret_cast<ngx_http_handler_pt *>(ngx_array_push(
      &core_main_config->phases[NGX_HTTP_SERVER_REWRITE_PHASE].handlers));
  if (handler == nullptr)
    return NGX_ERROR;
  *handler = before_response_handler;

  handler = reinterpret_cast<ngx_http_handler_pt *>(
      ngx_array_push(&core_main_config->phases[NGX_HTTP_LOG_PHASE].handlers));
  if (handler == nullptr)
    return NGX_ERROR;
  *handler = after_response_handler;
  return NGX_OK;
}

static void *ngx_http_opentracing_create_main_conf(ngx_conf_t *conf) {
  auto main_conf = ngx_pcalloc(conf->pool, sizeof(opentracing_main_conf_t));
  if (!main_conf)
    return nullptr;

  // set default options
  return main_conf;
}

static ngx_http_module_t ngx_http_opentracing_module_ctx = {
    nullptr,                               /* preconfiguration */
    ngx_http_opentracing_init,             /* postconfiguration */
    ngx_http_opentracing_create_main_conf, /* create main configuration */
    nullptr,                               /* init main configuration */
    nullptr,                               /* create server configuration */
    nullptr,                               /* merge server configuration */
    nullptr,                               /* create location configuration */
    nullptr                                /* merge location configuration */
};

// TODO: Need to add some additional options for more granular
// control over what locations are instrumented.
static ngx_command_t ngx_opentracing_commands[] = {
    {ngx_string("opentracing_tracer_options"),
     NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1, ngx_conf_set_str_slot,
     NGX_HTTP_MAIN_CONF_OFFSET,
     offsetof(opentracing_main_conf_t, tracer_options), nullptr},
    ngx_null_command};

ngx_module_t ngx_http_opentracing_module = {
    NGX_MODULE_V1,
    &ngx_http_opentracing_module_ctx, /* module context */
    ngx_opentracing_commands,         /* module directives */
    NGX_HTTP_MODULE,                  /* module type */
    nullptr,                          /* init master */
    nullptr,                          /* init module */
    nullptr,                          /* init process */
    nullptr,                          /* init thread */
    nullptr,                          /* exit thread */
    nullptr,                          /* exit process */
    nullptr,                          /* exit master */
    NGX_MODULE_V1_PADDING};
