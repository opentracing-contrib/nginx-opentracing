/* #include <lightstep/impl.h> */
/* #include <lightstep/recorder.h> */
/* #include <lightstep/tracer.h> */
#include <cstdlib>
#include <exception>
#include <iostream>
#include <unordered_map>
#include <utility>
#include "opentracing_conf.h"
#include "opentracing_handler.h"

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

extern ngx_module_t ngx_http_opentracing_module;
}

using ngx_opentracing::opentracing_main_conf_t;
using ngx_opentracing::opentracing_loc_conf_t;
using ngx_opentracing::opentracing_tag_t;
using ngx_opentracing::on_enter_block;
using ngx_opentracing::on_log_request;
using ngx_opentracing::NgxScript;

//------------------------------------------------------------------------------
// kDefaultOpentracingTags
//------------------------------------------------------------------------------
const std::pair<ngx_str_t, ngx_str_t> kDefaultOpenTracingTags[] = {
    {ngx_string("component"), ngx_string("nginx")},
    {ngx_string("nginx.worker_pid"), ngx_string("$pid")},
    {ngx_string("peer.address"), ngx_string("$remote_addr:$remote_port")},
    {ngx_string("http.method"), ngx_string("$request_method")},
    {ngx_string("http.url"), ngx_string("$scheme://$http_host$request_uri")}};

//------------------------------------------------------------------------------
// add_opentracing_tag
//------------------------------------------------------------------------------
static char *add_opentracing_tag(ngx_conf_t *cf, ngx_array_t *tags,
                                 ngx_str_t key, ngx_str_t value) {
  if (!tags) return static_cast<char *>(NGX_CONF_ERROR);

  auto tag = static_cast<opentracing_tag_t *>(ngx_array_push(tags));
  if (!tag) return static_cast<char *>(NGX_CONF_ERROR);

  ngx_memzero(tag, sizeof(opentracing_tag_t));
  if (tag->key_script.compile(cf, key) != NGX_OK)
    return static_cast<char *>(NGX_CONF_ERROR);
  if (tag->value_script.compile(cf, value) != NGX_OK)
    return static_cast<char *>(NGX_CONF_ERROR);

  return static_cast<char *>(NGX_CONF_OK);
}

//------------------------------------------------------------------------------
// set_opentracing_tag
//------------------------------------------------------------------------------
static char *set_opentracing_tag(ngx_conf_t *cf, ngx_command_t *command,
                                 void *conf) {
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(conf);
  if (!loc_conf->tags)
    loc_conf->tags = ngx_array_create(cf->pool, 1, sizeof(opentracing_tag_t));
  auto values = static_cast<ngx_str_t *>(cf->args->elts);
  return add_opentracing_tag(cf, loc_conf->tags, values[1], values[2]);
}

//------------------------------------------------------------------------------
// opentracing_module_init
//------------------------------------------------------------------------------
static ngx_int_t opentracing_module_init(ngx_conf_t *cf) {
  auto core_main_config = static_cast<ngx_http_core_main_conf_t *>(
      ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module));
  auto main_conf = static_cast<opentracing_main_conf_t *>(
      ngx_http_conf_get_module_main_conf(cf, ngx_http_opentracing_module));

  // Add handlers to create tracing data.
  auto handler = static_cast<ngx_http_handler_pt *>(ngx_array_push(
      &core_main_config->phases[NGX_HTTP_PREACCESS_PHASE].handlers));
  if (handler == nullptr) return NGX_ERROR;
  *handler = on_enter_block;

  handler = static_cast<ngx_http_handler_pt *>(
      ngx_array_push(&core_main_config->phases[NGX_HTTP_LOG_PHASE].handlers));
  if (handler == nullptr) return NGX_ERROR;
  *handler = on_log_request;

  // Add default span tags.
  const auto num_default_tags =
      sizeof(kDefaultOpenTracingTags) / sizeof(kDefaultOpenTracingTags[0]);
  if (num_default_tags == 0) return NGX_OK;
  main_conf->tags =
      ngx_array_create(cf->pool, num_default_tags, sizeof(opentracing_tag_t));
  if (!main_conf->tags) return NGX_ERROR;
  for (const auto &tag : kDefaultOpenTracingTags)
    if (add_opentracing_tag(cf, main_conf->tags, tag.first, tag.second) !=
        NGX_CONF_OK)
      return NGX_ERROR;
  return NGX_OK;
}

//------------------------------------------------------------------------------
// create_opentracing_main_conf
//------------------------------------------------------------------------------
static void *create_opentracing_main_conf(ngx_conf_t *conf) {
  auto main_conf = static_cast<opentracing_main_conf_t *>(
      ngx_pcalloc(conf->pool, sizeof(opentracing_main_conf_t)));
  if (!main_conf) return nullptr;
  return main_conf;
}

//------------------------------------------------------------------------------
// set_script
//------------------------------------------------------------------------------
static char *set_script(ngx_conf_t *cf, ngx_command_t *command,
                        NgxScript &script) {
  if (script.is_valid()) return const_cast<char *>("is duplicate");

  auto value = static_cast<ngx_str_t *>(cf->args->elts);
  auto pattern = &value[1];

  if (script.compile(cf, *pattern) != NGX_OK)
    return static_cast<char *>(NGX_CONF_ERROR);

  return static_cast<char *>(NGX_CONF_OK);
}

//------------------------------------------------------------------------------
// set_opentracing_operation_name
//------------------------------------------------------------------------------
static char *set_opentracing_operation_name(ngx_conf_t *cf,
                                            ngx_command_t *command,
                                            void *conf) {
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(conf);
  return set_script(cf, command, loc_conf->operation_name_script);
}

//------------------------------------------------------------------------------
// set_opentracing_location_operation_name
//------------------------------------------------------------------------------
static char *set_opentracing_location_operation_name(ngx_conf_t *cf,
                                                     ngx_command_t *command,
                                                     void *conf) {
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(conf);
  return set_script(cf, command, loc_conf->loc_operation_name_script);
}

//------------------------------------------------------------------------------
// create_opentracing_loc_conf
//------------------------------------------------------------------------------
static void *create_opentracing_loc_conf(ngx_conf_t *conf) {
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(
      ngx_pcalloc(conf->pool, sizeof(opentracing_loc_conf_t)));
  if (!loc_conf) return nullptr;

  loc_conf->enable = NGX_CONF_UNSET;
  loc_conf->enable_locations = NGX_CONF_UNSET;

  return loc_conf;
}

//------------------------------------------------------------------------------
// merge_opentracing_loc_conf
//------------------------------------------------------------------------------
static char *merge_opentracing_loc_conf(ngx_conf_t *, void *parent,
                                        void *child) {
  auto prev = static_cast<opentracing_loc_conf_t *>(parent);
  auto conf = static_cast<opentracing_loc_conf_t *>(child);

  ngx_conf_merge_value(conf->enable, prev->enable, 0);
  ngx_conf_merge_value(conf->enable_locations, prev->enable_locations, 1);

  if (prev->operation_name_script.is_valid() &&
      !conf->operation_name_script.is_valid())
    conf->operation_name_script = prev->operation_name_script;

  if (prev->loc_operation_name_script.is_valid() &&
      !conf->loc_operation_name_script.is_valid())
    conf->loc_operation_name_script = prev->loc_operation_name_script;

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
    if (!tags) return static_cast<char *>(NGX_CONF_ERROR);
    auto prev_tags = static_cast<opentracing_tag_t *>(prev->tags->elts);
    for (size_t i = 0; i < prev->tags->nelts; ++i) tags[i] = prev_tags[i];
  }

  return NGX_CONF_OK;
}

//------------------------------------------------------------------------------
// opentracing_module_ctx
//------------------------------------------------------------------------------
static ngx_http_module_t opentracing_module_ctx = {
    nullptr,                      /* preconfiguration */
    opentracing_module_init,      /* postconfiguration */
    create_opentracing_main_conf, /* create main configuration */
    nullptr,                      /* init main configuration */
    nullptr,                      /* create server configuration */
    nullptr,                      /* merge server configuration */
    create_opentracing_loc_conf,  /* create location configuration */
    merge_opentracing_loc_conf    /* merge location configuration */
};

//------------------------------------------------------------------------------
// opentracing_commands
//------------------------------------------------------------------------------
static ngx_command_t opentracing_commands[] = {
// Customization point: A tracer implentation will expose this file and
// list any commands specific for that tracer.
#include <ngx_opentracing_tracer_commands.def>
    {ngx_string("opentracing"),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF |
         NGX_CONF_TAKE1,
     ngx_conf_set_flag_slot, NGX_HTTP_LOC_CONF_OFFSET,
     offsetof(opentracing_loc_conf_t, enable), nullptr},
    {ngx_string("opentracing_trace_locations"),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF |
         NGX_CONF_TAKE1,
     ngx_conf_set_flag_slot, NGX_HTTP_LOC_CONF_OFFSET,
     offsetof(opentracing_loc_conf_t, enable_locations), nullptr},
    {ngx_string("opentracing_operation_name"),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF |
         NGX_CONF_TAKE1,
     set_opentracing_operation_name, NGX_HTTP_LOC_CONF_OFFSET, 0, nullptr},
    {ngx_string("opentracing_location_operation_name"),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF |
         NGX_CONF_TAKE1,
     set_opentracing_location_operation_name, NGX_HTTP_LOC_CONF_OFFSET, 0,
     nullptr},
    {ngx_string("opentracing_tag"),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF |
         NGX_CONF_TAKE2,
     set_opentracing_tag, NGX_HTTP_LOC_CONF_OFFSET, 0, nullptr},
    ngx_null_command};

//------------------------------------------------------------------------------
// ngx_http_opentracing_module
//------------------------------------------------------------------------------
ngx_module_t ngx_http_opentracing_module = {
    NGX_MODULE_V1,
    &opentracing_module_ctx, /* module context */
    opentracing_commands,    /* module directives */
    NGX_HTTP_MODULE,         /* module type */
    nullptr,                 /* init master */
    nullptr,                 /* init module */
    nullptr,                 /* init process */
    nullptr,                 /* init thread */
    nullptr,                 /* exit thread */
    nullptr,                 /* exit process */
    nullptr,                 /* exit master */
    NGX_MODULE_V1_PADDING};
