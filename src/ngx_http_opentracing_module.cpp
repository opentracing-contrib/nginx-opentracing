#include "ngx_http_opentracing_conf.h"
#include "opentracing_request_processor.h"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <lightstep/impl.h>
#include <lightstep/recorder.h>
#include <lightstep/tracer.h>
#include <unordered_map>
#include <utility>

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
using ngx_opentracing::opentracing_tag_t;

const std::pair<ngx_str_t, ngx_str_t> kDefaultOpenTracingTags[] = {
    {ngx_string("component"), ngx_string("nginx")},
    {ngx_string("nginx.worker_pid"), ngx_string("$pid")},
    {ngx_string("peer.address"), ngx_string("$remote_addr:$remote_port")},
    {ngx_string("http.method"), ngx_string("$request_method")},
    {ngx_string("http.url"), ngx_string("$scheme://$http_host$request_uri")}};

static ngx_int_t compile_script(ngx_conf_t *cf, ngx_uint_t num_variables,
                                ngx_str_t *script, ngx_array_t **lengths,
                                ngx_array_t **values) {
  ngx_http_script_compile_t script_compile;
  ngx_memzero(&script_compile, sizeof(ngx_http_script_compile_t));
  script_compile.cf = cf;
  script_compile.source = script;
  script_compile.lengths = lengths;
  script_compile.values = values;
  script_compile.variables = num_variables;
  script_compile.complete_lengths = 1;
  script_compile.complete_values = 1;

  return ngx_http_script_compile(&script_compile);
}

static ngx_int_t compile_script(ngx_conf_t *cf, ngx_str_t *script,
                                ngx_array_t **lengths, ngx_array_t **values) {
  auto num_variables = ngx_http_script_variables_count(script);
  return compile_script(cf, num_variables, script, lengths, values);
}

static char *add_opentracing_tag(ngx_conf_t *cf, ngx_array_t *tags,
                                 ngx_str_t key, ngx_str_t value) {
  if (!tags)
    return static_cast<char *>(NGX_CONF_ERROR);

  auto tag = static_cast<opentracing_tag_t *>(ngx_array_push(tags));
  if (!tag)
    return static_cast<char *>(NGX_CONF_ERROR);

  ngx_memzero(tag, sizeof(opentracing_tag_t));

  if (compile_script(cf, &key, &tag->key_lengths, &tag->key_values) != NGX_OK)
    return static_cast<char *>(NGX_CONF_ERROR);

  if (compile_script(cf, &value, &tag->value_lengths, &tag->value_values) !=
      NGX_OK)
    return static_cast<char *>(NGX_CONF_ERROR);

  return static_cast<char *>(NGX_CONF_OK);
}

static OpenTracingRequestProcessor &
get_opentracing_request_processor(ngx_http_request_t *request) {
  static auto request_processor = [request] {
    auto conf = static_cast<opentracing_main_conf_t *>(
        ngx_http_get_module_main_conf(request, ngx_http_opentracing_module));
    return OpenTracingRequestProcessor{conf->tracer_options};
  }();
  return request_processor;
}

static ngx_int_t before_response_handler(ngx_http_request_t *request) {
  auto core_loc_conf = static_cast<ngx_http_core_loc_conf_t *>(
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

static ngx_int_t ngx_http_opentracing_init(ngx_conf_t *cf) {
  auto core_main_config = static_cast<ngx_http_core_main_conf_t *>(
      ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module));
  auto main_conf = static_cast<opentracing_main_conf_t *>(
      ngx_http_conf_get_module_main_conf(cf, ngx_http_opentracing_module));

  // Add handlers to create tracing data.
  auto handler = static_cast<ngx_http_handler_pt *>(ngx_array_push(
      &core_main_config->phases[NGX_HTTP_PREACCESS_PHASE].handlers));
  if (handler == nullptr)
    return NGX_ERROR;
  *handler = before_response_handler;

  handler = static_cast<ngx_http_handler_pt *>(
      ngx_array_push(&core_main_config->phases[NGX_HTTP_LOG_PHASE].handlers));
  if (handler == nullptr)
    return NGX_ERROR;
  *handler = after_response_handler;

  // Add default span tags.
  const auto num_default_tags =
      sizeof(kDefaultOpenTracingTags) / sizeof(kDefaultOpenTracingTags[0]);
  if (num_default_tags == 0)
    return NGX_OK;
  main_conf->tags =
      ngx_array_create(cf->pool, num_default_tags, sizeof(opentracing_tag_t));
  if (!main_conf->tags)
    return NGX_ERROR;
  for (const auto &tag : kDefaultOpenTracingTags)
    if (add_opentracing_tag(cf, main_conf->tags, tag.first, tag.second) !=
        NGX_CONF_OK)
      return NGX_ERROR;
  return NGX_OK;
}

static void *ngx_http_opentracing_create_main_conf(ngx_conf_t *conf) {
  auto main_conf = static_cast<opentracing_main_conf_t *>(
      ngx_pcalloc(conf->pool, sizeof(opentracing_main_conf_t)));
  if (!main_conf)
    return nullptr;
  return main_conf;
}

static char *ngx_http_opentracing_operation_name(ngx_conf_t *cf,
                                                 ngx_command_t *command,
                                                 void *conf) {
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(conf);
  if (loc_conf->operation_name.data)
    return const_cast<char *>("is duplicate");

  auto value = static_cast<ngx_str_t *>(cf->args->elts);
  auto operation_name = &value[1];

  loc_conf->operation_name = *operation_name;

  auto num_variables = ngx_http_script_variables_count(operation_name);
  if (num_variables == 0)
    return static_cast<char *>(NGX_CONF_OK);

  if (compile_script(cf, num_variables, operation_name,
                     &loc_conf->operation_name_lengths,
                     &loc_conf->operation_name_values) != NGX_OK)
    return static_cast<char *>(NGX_CONF_ERROR);

  return static_cast<char *>(NGX_CONF_OK);
}

static char *ngx_http_opentracing_tag(ngx_conf_t *cf, ngx_command_t *command,
                                      void *conf) {
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(conf);
  if (!loc_conf->tags)
    loc_conf->tags = ngx_array_create(cf->pool, 1, sizeof(opentracing_tag_t));
  auto values = static_cast<ngx_str_t *>(cf->args->elts);
  return add_opentracing_tag(cf, loc_conf->tags, values[1], values[2]);
}

static void *ngx_http_opentracing_create_loc_conf(ngx_conf_t *conf) {
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(
      ngx_pcalloc(conf->pool, sizeof(opentracing_loc_conf_t)));
  if (!loc_conf)
    return nullptr;

  loc_conf->enable = NGX_CONF_UNSET;
  loc_conf->operation_name = {0, nullptr};
  loc_conf->operation_name_lengths = nullptr;
  loc_conf->operation_name_values = nullptr;

  loc_conf->tags = nullptr;

  return loc_conf;
}

static char *ngx_http_opentracing_merge_loc_conf(ngx_conf_t *, void *parent,
                                                 void *child) {
  auto prev = static_cast<opentracing_loc_conf_t *>(parent);
  auto conf = static_cast<opentracing_loc_conf_t *>(child);

  ngx_conf_merge_value(conf->enable, prev->enable, 0);

  // Create a new array that joins `prev->tags` and `conf->tags`. Since tags
  // are set consecutively and setting a tag with the same key as a previous
  // one overwrites it, we need to ensure that the tags in `conf->tags` come
  // after `prev->tags` so as to keep the value from the most specific
  // configuration.
  if (prev->tags && !conf->tags) {
    conf->tags = prev->tags;
  } else if (prev->tags && conf->tags) {
    std::swap(prev->tags, conf->tags);
    auto tags = static_cast<opentracing_tag_t *>(
        ngx_array_push_n(conf->tags, prev->tags->nelts));
    if (!tags)
      return static_cast<char *>(NGX_CONF_ERROR);
    auto prev_tags = static_cast<opentracing_tag_t *>(prev->tags->elts);
    for (size_t i = 0; i < prev->tags->nelts; ++i)
      tags[i] = prev_tags[i];
  }

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
