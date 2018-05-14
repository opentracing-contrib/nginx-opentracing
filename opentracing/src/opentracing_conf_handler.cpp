#include "opentracing_conf_handler.h"

namespace ngx_opentracing {
/* The eight fixed arguments */

static ngx_uint_t argument_number[] = {
    NGX_CONF_NOARGS, NGX_CONF_TAKE1, NGX_CONF_TAKE2, NGX_CONF_TAKE3,
    NGX_CONF_TAKE4,  NGX_CONF_TAKE5, NGX_CONF_TAKE6, NGX_CONF_TAKE7};

//------------------------------------------------------------------------------
// opentracing_conf_handler
//------------------------------------------------------------------------------
ngx_int_t opentracing_conf_handler(ngx_conf_t *cf, ngx_int_t last) noexcept {
  char *rv;
  void *conf, **confp;
  ngx_uint_t i, found;
  ngx_str_t *name;
  ngx_command_t *cmd;

  name = static_cast<ngx_str_t *>(cf->args->elts);

  found = 0;

  for (i = 0; cf->cycle->modules[i]; i++) {
    cmd = cf->cycle->modules[i]->commands;
    if (cmd == NULL) {
      continue;
    }

    for (/* void */; cmd->name.len; cmd++) {
      if (name->len != cmd->name.len) {
        continue;
      }

      if (ngx_strcmp(name->data, cmd->name.data) != 0) {
        continue;
      }

      found = 1;

      if (cf->cycle->modules[i]->type != NGX_CONF_MODULE &&
          cf->cycle->modules[i]->type != cf->module_type) {
        continue;
      }

      /* is the directive's location right ? */

      if (!(cmd->type & cf->cmd_type)) {
        continue;
      }

      if (!(cmd->type & NGX_CONF_BLOCK) && last != NGX_OK) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "directive \"%s\" is not terminated by \";\"",
                           name->data);
        return NGX_ERROR;
      }

      if ((cmd->type & NGX_CONF_BLOCK) && last != NGX_CONF_BLOCK_START) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "directive \"%s\" has no opening \"{\"", name->data);
        return NGX_ERROR;
      }

      /* is the directive's argument count right ? */

      if (!(cmd->type & NGX_CONF_ANY)) {
        if (cmd->type & NGX_CONF_FLAG) {
          if (cf->args->nelts != 2) {
            goto invalid;
          }

        } else if (cmd->type & NGX_CONF_1MORE) {
          if (cf->args->nelts < 2) {
            goto invalid;
          }

        } else if (cmd->type & NGX_CONF_2MORE) {
          if (cf->args->nelts < 3) {
            goto invalid;
          }

        } else if (cf->args->nelts > NGX_CONF_MAX_ARGS) {
          goto invalid;

        } else if (!(cmd->type & argument_number[cf->args->nelts - 1])) {
          goto invalid;
        }
      }

      /* set up the directive's configuration context */

      conf = NULL;

      if (cmd->type & NGX_DIRECT_CONF) {
        conf = ((void **)cf->ctx)[cf->cycle->modules[i]->index];

      } else if (cmd->type & NGX_MAIN_CONF) {
        conf = &(((void **)cf->ctx)[cf->cycle->modules[i]->index]);

      } else if (cf->ctx) {
        confp = static_cast<void **>(*(void **)((char *)cf->ctx + cmd->conf));

        if (confp) {
          conf = confp[cf->cycle->modules[i]->ctx_index];
        }
      }

      rv = cmd->set(cf, cmd, conf);

      if (rv == NGX_CONF_OK) {
        return NGX_OK;
      }

      if (rv == NGX_CONF_ERROR) {
        return NGX_ERROR;
      }

      ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "\"%s\" directive %s",
                         name->data, rv);

      return NGX_ERROR;
    }
  }

  if (found) {
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "\"%s\" directive is not allowed here", name->data);

    return NGX_ERROR;
  }

  ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unknown directive \"%s\"",
                     name->data);

  return NGX_ERROR;

invalid:

  ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                     "invalid number of arguments in \"%s\" directive",
                     name->data);

  return NGX_ERROR;
}
}  // namespace ngx_opentracing
