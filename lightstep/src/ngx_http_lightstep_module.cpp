#include <lightstep/tracer.h>
#include <opentracing/tracer.h>
#include <cstdlib>

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

extern ngx_module_t ngx_http_lightstep_module;
}

//------------------------------------------------------------------------------
// to_string
//------------------------------------------------------------------------------
static inline std::string to_string(const ngx_str_t &ngx_str) {
  return {reinterpret_cast<char *>(ngx_str.data), ngx_str.len};
}

//------------------------------------------------------------------------------
// lightstep_main_conf_t
//------------------------------------------------------------------------------
struct lightstep_main_conf_t {
  ngx_str_t access_token;
  ngx_str_t component_name;
  ngx_str_t collector_host;
  ngx_flag_t collector_plaintext = NGX_CONF_UNSET;
  ngx_str_t collector_port;
};

//------------------------------------------------------------------------------
// lightstep_module_init
//------------------------------------------------------------------------------
static ngx_int_t lightstep_module_init(ngx_conf_t *cf) {
  auto main_conf = static_cast<lightstep_main_conf_t *>(
      ngx_http_conf_get_module_main_conf(cf, ngx_http_lightstep_module));
  std::cout << "init lightstep module" << std::endl;
  // Validate the configuration
  if (!main_conf->access_token.data) {
    ngx_log_error(NGX_LOG_ERR, cf->log, 0,
                  "`lighstep_access_token` must be specified");
    return NGX_ERROR;
  }
  return NGX_OK;
}

//------------------------------------------------------------------------------
// lightstep_init_worker
//------------------------------------------------------------------------------
static ngx_int_t lightstep_init_worker(ngx_cycle_t *cycle) {
  auto main_conf = static_cast<lightstep_main_conf_t *>(
      ngx_http_cycle_get_module_main_conf(cycle, ngx_http_lightstep_module));
  lightstep::LightStepTracerOptions tracer_options;
  if (!main_conf->access_token.data) {
    ngx_log_error(NGX_LOG_ERR, cycle->log, 0,
                  "`lighstep_access_token` must be specified");
    return NGX_ERROR;
  }
  tracer_options.access_token = to_string(main_conf->access_token);
  if (main_conf->collector_host.data)
    tracer_options.collector_host = to_string(main_conf->collector_host);
  if (main_conf->collector_port.data)
    // TODO: Check for errors here?
    tracer_options.collector_port =
        std::stoi(to_string(main_conf->collector_port));
  if (main_conf->collector_plaintext != NGX_CONF_UNSET)
    tracer_options.collector_plaintext = main_conf->collector_plaintext;
  if (main_conf->component_name.data)
    tracer_options.component_name = to_string(main_conf->component_name);
  else
    tracer_options.component_name = "nginx";
  auto tracer = lightstep::MakeLightStepTracer(std::move(tracer_options));
  if (!tracer) {
    ngx_log_error(NGX_LOG_ERR, cycle->log, 0,
                  "Failed to create LightStep tracer");
    return NGX_OK;
  }
  opentracing::Tracer::InitGlobal(std::move(tracer));
  return NGX_OK;
}

//------------------------------------------------------------------------------
// create_lightstep_main_conf
//------------------------------------------------------------------------------
static void *create_lightstep_main_conf(ngx_conf_t *conf) {
  auto main_conf = static_cast<lightstep_main_conf_t *>(
      ngx_pcalloc(conf->pool, sizeof(lightstep_main_conf_t)));
  // Default initialize members.
  *main_conf = lightstep_main_conf_t();
  if (!main_conf) return nullptr;
  return main_conf;
}

//------------------------------------------------------------------------------
// lightstep_module_ctx
//------------------------------------------------------------------------------
static ngx_http_module_t lightstep_module_ctx = {
    nullptr,                    /* preconfiguration */
    lightstep_module_init,      /* postconfiguration */
    create_lightstep_main_conf, /* create main configuration */
    nullptr,                    /* init main configuration */
    nullptr,                    /* create server configuration */
    nullptr,                    /* merge server configuration */
    nullptr,                    /* create location configuration */
    nullptr                     /* merge location configuration */
};

//------------------------------------------------------------------------------
// lightstep_commands
//------------------------------------------------------------------------------
static ngx_command_t lightstep_commands[] = {
    {ngx_string("lightstep_access_token"), NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
     ngx_conf_set_str_slot, NGX_HTTP_MAIN_CONF_OFFSET,
     offsetof(lightstep_main_conf_t, access_token), nullptr},
    {ngx_string("lightstep_component_name"),
     NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1, ngx_conf_set_str_slot,
     NGX_HTTP_MAIN_CONF_OFFSET, offsetof(lightstep_main_conf_t, component_name),
     nullptr},
    {ngx_string("lightstep_collector_host"),
     NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1, ngx_conf_set_str_slot,
     NGX_HTTP_MAIN_CONF_OFFSET, offsetof(lightstep_main_conf_t, collector_host),
     nullptr},
    {ngx_string("lightstep_collector_plaintext"),
     NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1, ngx_conf_set_flag_slot,
     NGX_HTTP_MAIN_CONF_OFFSET,
     offsetof(lightstep_main_conf_t, collector_plaintext), nullptr},
    {ngx_string("lightstep_collector_port"),
     NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1, ngx_conf_set_str_slot,
     NGX_HTTP_MAIN_CONF_OFFSET, offsetof(lightstep_main_conf_t, collector_port),
     nullptr}};

//------------------------------------------------------------------------------
// ngx_http_lightstep_module
//------------------------------------------------------------------------------
ngx_module_t ngx_http_lightstep_module = {
    NGX_MODULE_V1,
    &lightstep_module_ctx, /* module context */
    lightstep_commands,    /* module directives */
    NGX_HTTP_MODULE,       /* module type */
    nullptr,               /* init master */
    nullptr,               /* init module */
    lightstep_init_worker, /* init process */
    nullptr,               /* init thread */
    nullptr,               /* exit thread */
    nullptr,               /* exit process */
    nullptr,               /* exit master */
    NGX_MODULE_V1_PADDING};
