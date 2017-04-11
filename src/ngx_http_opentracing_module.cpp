#include "ngx_http_opentracing_conf.h"
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
using ngx_opentracing::opentracing_main_conf_t;
using ngx_opentracing::opentracing_loc_conf_t;

static OpenTracingRequestProcessor &
get_opentracing_request_processor(ngx_http_request_t *request) {
  static auto request_processor = [request] {
    auto conf = reinterpret_cast<opentracing_main_conf_t *>(
        ngx_http_get_module_main_conf(request, ngx_http_opentracing_module));
    return OpenTracingRequestProcessor{conf->tracer_options};
  }();
  return request_processor;
}

static ngx_int_t before_response_handler(ngx_http_request_t *request) {
  auto core_loc_conf = reinterpret_cast<ngx_http_core_loc_conf_t *>(
      ngx_http_get_module_loc_conf(request, ngx_http_core_module));
  std::cerr << "before: "
            << std::string{reinterpret_cast<char *>(request->uri.data),
                           request->uri.len}
            << " - "
            << std::string{reinterpret_cast<char *>(core_loc_conf->name.data),
                           core_loc_conf->name.len}
            << " - " << request << "\n";
  get_opentracing_request_processor(request).before_response(request);

  return NGX_DECLINED;
}

static ngx_int_t after_response_handler(ngx_http_request_t *request) {
  std::cerr << "after: "
            << std::string{reinterpret_cast<char *>(request->uri.data),
                           request->uri.len}
            << " - " << request << "\n";

  get_opentracing_request_processor(request).after_response(request);

  return NGX_DECLINED;
}

static ngx_int_t ngx_http_opentracing_init(ngx_conf_t *config) {
  auto core_main_config = reinterpret_cast<ngx_http_core_main_conf_t *>(
      ngx_http_conf_get_module_main_conf(config, ngx_http_core_module));

  auto handler = reinterpret_cast<ngx_http_handler_pt *>(ngx_array_push(
      &core_main_config->phases[NGX_HTTP_PREACCESS_PHASE].handlers));
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
  auto main_conf = reinterpret_cast<opentracing_main_conf_t *>(
      ngx_pcalloc(conf->pool, sizeof(opentracing_main_conf_t)));
  if (!main_conf)
    return nullptr;
  return main_conf;
}

static char *ngx_http_opentracing_operation_name(ngx_conf_t *cf,
                                                 ngx_command_t *command,
                                                 void *conf) {
  auto loc_conf = reinterpret_cast<opentracing_loc_conf_t *>(conf);
  if (loc_conf->operation_name.data)
    return const_cast<char *>("is duplicate");

  auto value = reinterpret_cast<ngx_str_t *>(cf->args->elts);
  auto operation_name = &value[1];

  loc_conf->operation_name = *operation_name;
  loc_conf->operation_name_lengths = nullptr;
  loc_conf->operation_name_values = nullptr;

  auto num_variables = ngx_http_script_variables_count(operation_name);
  if (num_variables == 0)
    return reinterpret_cast<char *>(NGX_CONF_OK);

  ngx_http_script_compile_t script_compile;
  ngx_memzero(&script_compile, sizeof(ngx_http_script_compile_t));
  script_compile.cf = cf;
  script_compile.source = operation_name;
  script_compile.lengths = &loc_conf->operation_name_lengths;
  script_compile.values = &loc_conf->operation_name_values;
  script_compile.variables = num_variables;
  script_compile.complete_lengths = 1;
  script_compile.complete_values = 1;

  if (ngx_http_script_compile(&script_compile) != NGX_OK)
    return reinterpret_cast<char *>(NGX_CONF_ERROR);

  return reinterpret_cast<char *>(NGX_CONF_OK);
}

static char *ngx_http_opentracing_tag(ngx_conf_t *cf, ngx_command_t *command,
                                      void *conf) {
  return reinterpret_cast<char *>(NGX_CONF_OK);
}

static void *ngx_http_opentracing_create_loc_conf(ngx_conf_t *conf) {
  auto loc_conf = reinterpret_cast<opentracing_loc_conf_t *>(
      ngx_pcalloc(conf->pool, sizeof(opentracing_loc_conf_t)));
  if (!loc_conf)
    return nullptr;

  loc_conf->enable = NGX_CONF_UNSET;
  loc_conf->operation_name = {0, nullptr};

  return loc_conf;
}

static char *ngx_http_opentracing_merge_loc_conf(ngx_conf_t *, void *parent,
                                                 void *child) {
  auto prev = reinterpret_cast<opentracing_loc_conf_t *>(parent);
  auto conf = reinterpret_cast<opentracing_loc_conf_t *>(child);

  ngx_conf_merge_value(conf->enable, prev->enable, 0);

  return NGX_CONF_OK;
}

static ngx_http_module_t ngx_http_opentracing_module_ctx = {
    nullptr,                               /* preconfiguration */
    ngx_http_opentracing_init,             /* postconfiguration */
    ngx_http_opentracing_create_main_conf, /* create main configuration */
    nullptr,                               /* init main configuration */
    nullptr,                               /* create server configuration */
    nullptr,                               /* merge server configuration */
    ngx_http_opentracing_create_loc_conf,  /* create location configuration */
    ngx_http_opentracing_merge_loc_conf    /* merge location configuration */
};

static ngx_command_t ngx_opentracing_commands[] = {
#include <opentracing_tracer_options_command.def>
    {ngx_string("opentracing"),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
     ngx_conf_set_flag_slot, NGX_HTTP_LOC_CONF_OFFSET,
     offsetof(opentracing_loc_conf_t, enable), nullptr},
    {ngx_string("opentracing_operation_name"),
     NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1, ngx_http_opentracing_operation_name,
     NGX_HTTP_LOC_CONF_OFFSET, 0, nullptr},
    {ngx_string("opentracing_tag"),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF |
         NGX_CONF_TAKE2,
     ngx_http_opentracing_tag, NGX_HTTP_LOC_CONF_OFFSET, 0, nullptr},
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
