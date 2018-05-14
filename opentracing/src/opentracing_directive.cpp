#include "opentracing_directive.h"

#include "discover_span_context_keys.h"
#include "ngx_script.h"
#include "opentracing_conf.h"
#include "opentracing_conf_handler.h"
#include "opentracing_variable.h"
#include "utility.h"

#include <opentracing/string_view.h>

#include <algorithm>
#include <iostream>
#include <string>

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// set_script
//------------------------------------------------------------------------------
static char *set_script(ngx_conf_t *cf, ngx_command_t *command,
                        NgxScript &script) noexcept {
  if (script.is_valid()) return const_cast<char *>("is duplicate");

  auto value = static_cast<ngx_str_t *>(cf->args->elts);
  auto pattern = &value[1];

  if (script.compile(cf, *pattern) != NGX_OK)
    return static_cast<char *>(NGX_CONF_ERROR);

  return static_cast<char *>(NGX_CONF_OK);
}

//------------------------------------------------------------------------------
// make_span_context_value_variable
//------------------------------------------------------------------------------
static ngx_str_t make_span_context_value_variable(ngx_pool_t *pool, int index) {
  std::string result =
      "$" OPENTRACING_SPAN_CONTEXT_HEADER_VALUE + std::to_string(index);
  return to_ngx_str(pool, result);
}

//------------------------------------------------------------------------------
// add_opentracing_tag
//------------------------------------------------------------------------------
char *add_opentracing_tag(ngx_conf_t *cf, ngx_array_t *tags, ngx_str_t key,
                          ngx_str_t value) noexcept {
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
// propagate_opentracing_context
//------------------------------------------------------------------------------
char *propagate_opentracing_context(ngx_conf_t *cf, ngx_command_t * /*command*/,
                                    void * /*conf*/) noexcept try {
  auto main_conf = static_cast<opentracing_main_conf_t *>(
      ngx_http_conf_get_module_main_conf(cf, ngx_http_opentracing_module));
  if (main_conf->span_context_keys == nullptr) {
    return static_cast<char *>(NGX_CONF_OK);
  }
  auto keys = static_cast<opentracing::string_view *>(
      main_conf->span_context_keys->elts);
  auto num_keys = static_cast<int>(main_conf->span_context_keys->nelts);

  auto old_args = cf->args;

  ngx_str_t args[] = {ngx_string("proxy_set_header"), {}, {}};
  ngx_array_t args_array;
  args_array.elts = static_cast<void *>(&args);
  args_array.nelts = 3;

  cf->args = &args_array;
  for (int key_index = 0; key_index < num_keys; ++key_index) {
    args[1] = ngx_str_t{keys[key_index].size(),
                        reinterpret_cast<unsigned char *>(
                            const_cast<char *>(keys[key_index].data()))};
    args[2] = make_span_context_value_variable(cf->pool, key_index);
    auto rcode = opentracing_conf_handler(cf, 0);
    if (rcode != NGX_OK) {
      cf->args = old_args;
      return static_cast<char *>(NGX_CONF_ERROR);
    }
  }
  cf->args = old_args;
  return static_cast<char *>(NGX_CONF_OK);
} catch (const std::exception& e) {
  ngx_log_error(NGX_LOG_ERR, cf->log, 0,
                "opentracing_propatate_context failed: %s", e.what());
  return static_cast<char *>(NGX_CONF_ERROR);
}

//------------------------------------------------------------------------------
// set_opentracing_tag
//------------------------------------------------------------------------------
char *set_opentracing_tag(ngx_conf_t *cf, ngx_command_t *command,
                          void *conf) noexcept {
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(conf);
  if (!loc_conf->tags)
    loc_conf->tags = ngx_array_create(cf->pool, 1, sizeof(opentracing_tag_t));
  auto values = static_cast<ngx_str_t *>(cf->args->elts);
  return add_opentracing_tag(cf, loc_conf->tags, values[1], values[2]);
}

//------------------------------------------------------------------------------
// set_opentracing_operation_name
//------------------------------------------------------------------------------
char *set_opentracing_operation_name(ngx_conf_t *cf, ngx_command_t *command,
                                     void *conf) noexcept {
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(conf);
  return set_script(cf, command, loc_conf->operation_name_script);
}

//------------------------------------------------------------------------------
// set_opentracing_location_operation_name
//------------------------------------------------------------------------------
char *set_opentracing_location_operation_name(ngx_conf_t *cf,
                                              ngx_command_t *command,
                                              void *conf) noexcept {
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(conf);
  return set_script(cf, command, loc_conf->loc_operation_name_script);
}

//------------------------------------------------------------------------------
// set_tracer
//------------------------------------------------------------------------------
char *set_tracer(ngx_conf_t *cf, ngx_command_t *command,
                 void *conf) noexcept try {
  auto main_conf = static_cast<opentracing_main_conf_t *>(
      ngx_http_conf_get_module_main_conf(cf, ngx_http_opentracing_module));
  auto values = static_cast<ngx_str_t *>(cf->args->elts);
  main_conf->tracer_library = values[1];
  main_conf->tracer_conf_file = values[2];
  main_conf->span_context_keys = discover_span_context_keys(
      cf->pool, cf->log, to_string(main_conf->tracer_library).c_str(),
      to_string(main_conf->tracer_conf_file).c_str());
  if (main_conf->span_context_keys == nullptr) {
    return static_cast<char *>(NGX_CONF_ERROR);
  }
  return static_cast<char *>(NGX_CONF_OK);
} catch (const std::exception &e) {
  ngx_log_error(NGX_LOG_ERR, cf->log, 0, "set_tracer failed: %s", e.what());
  return static_cast<char *>(NGX_CONF_ERROR);
}
}  // namespace ngx_opentracing
