#include "opentracing_directive.h"

#include "ngx_script.h"
#include "opentracing_conf.h"
#include "opentracing_conf_handler.h"
#include "opentracing_variable.h"

#include <algorithm>
#include <iostream>

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
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
// add_dollar_variable_prefix
//------------------------------------------------------------------------------
static ngx_str_t add_dollar_variable_prefix(ngx_pool_t *pool,
                                            const std::string &s) {
  ngx_str_t result;
  result.len = s.size() + 1;
  result.data = static_cast<unsigned char *>(ngx_palloc(pool, result.len));
  if (!result.data) return {0, nullptr};
  *result.data = '$';
  std::copy(s.begin(), s.end(), result.data + 1);
  return result;
}

//------------------------------------------------------------------------------
// add_opentracing_tag
//------------------------------------------------------------------------------
char *add_opentracing_tag(ngx_conf_t *cf, ngx_array_t *tags, ngx_str_t key,
                          ngx_str_t value) {
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
                                    void * /*conf*/) {
  std::cout << "nufnufwoof\n";
  auto old_args = cf->args;

  ngx_str_t args[] = {ngx_string("proxy_set_header"), {}, {}};
  ngx_array_t args_array;
  args_array.elts = static_cast<void *>(&args);
  args_array.nelts = 3;

  cf->args = &args_array;
  for (auto &name : get_context_propagation_header_variable_names()) {
    args[1] = add_dollar_variable_prefix(cf->pool, name.first);
    args[2] = add_dollar_variable_prefix(cf->pool, name.second);
    auto rcode = opentracing_conf_handler(cf, 0);
    if (rcode != NGX_OK) {
      cf->args = old_args;
      return static_cast<char *>(NGX_CONF_ERROR);
    }
  }
  cf->args = old_args;

  return static_cast<char *>(NGX_CONF_OK);
}

//------------------------------------------------------------------------------
// set_opentracing_tag
//------------------------------------------------------------------------------
char *set_opentracing_tag(ngx_conf_t *cf, ngx_command_t *command, void *conf) {
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
                                     void *conf) {
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(conf);
  return set_script(cf, command, loc_conf->operation_name_script);
}

//------------------------------------------------------------------------------
// set_opentracing_location_operation_name
//------------------------------------------------------------------------------
char *set_opentracing_location_operation_name(ngx_conf_t *cf,
                                              ngx_command_t *command,
                                              void *conf) {
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(conf);
  return set_script(cf, command, loc_conf->loc_operation_name_script);
}

//------------------------------------------------------------------------------
// set_tracer
//------------------------------------------------------------------------------
char *set_tracer(ngx_conf_t *cf, ngx_command_t *command, void *conf) {
  auto main_conf = static_cast<opentracing_main_conf_t *>(
      ngx_http_conf_get_module_main_conf(cf, ngx_http_opentracing_module));
  auto values = static_cast<ngx_str_t *>(cf->args->elts);
  main_conf->tracer_library = values[1];
  main_conf->tracer_conf_file = values[2];
  return static_cast<char *>(NGX_CONF_OK);
}
}  // namespace ngx_opentracing
