#include <opentracing/tracer.h>
#include <zipkin/opentracing.h>
#include <cstdlib>

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

extern ngx_module_t ngx_http_zipkin_module;
}

//------------------------------------------------------------------------------
// to_string
//------------------------------------------------------------------------------
static inline std::string to_string(const ngx_str_t &ngx_str) {
  return {reinterpret_cast<char *>(ngx_str.data), ngx_str.len};
}

//------------------------------------------------------------------------------
// zipkin_main_conf_t
//------------------------------------------------------------------------------
struct zipkin_main_conf_t {
  ngx_str_t collector_host;
  ngx_str_t collector_port;
  ngx_str_t service_name;
  ngx_str_t sample_rate;
};

//------------------------------------------------------------------------------
// zipkin_init_worker
//------------------------------------------------------------------------------
static ngx_int_t zipkin_init_worker(ngx_cycle_t *cycle) {
  auto main_conf = static_cast<zipkin_main_conf_t *>(
      ngx_http_cycle_get_module_main_conf(cycle, ngx_http_zipkin_module));
  zipkin::ZipkinOtTracerOptions tracer_options;
  if (main_conf->collector_host.data)
    tracer_options.collector_host = to_string(main_conf->collector_host);
  if (main_conf->collector_port.data)
    // TODO: Check for errors here?
    tracer_options.collector_port =
        std::stoi(to_string(main_conf->collector_port));
  if (main_conf->service_name.data)
    tracer_options.service_name = to_string(main_conf->service_name);
  else
    tracer_options.service_name = "nginx";
  if (main_conf->sample_rate.data)
    tracer_options.sample_rate = std::stod(to_string(main_conf->sample_rate));
  else
    tracer_options.sample_rate = 1.0;
  
  auto tracer = makeZipkinOtTracer(tracer_options);
  if (!tracer) {
    ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "Failed to create Zipkin tracer");
    return NGX_OK;
  }
  opentracing::Tracer::InitGlobal(std::move(tracer));
  return NGX_OK;
}

//------------------------------------------------------------------------------
// create_zipkin_main_conf
//------------------------------------------------------------------------------
static void *create_zipkin_main_conf(ngx_conf_t *conf) {
  auto main_conf = static_cast<zipkin_main_conf_t *>(
      ngx_pcalloc(conf->pool, sizeof(zipkin_main_conf_t)));
  // Default initialize members.
  *main_conf = zipkin_main_conf_t();
  if (!main_conf) return nullptr;
  return main_conf;
}

//------------------------------------------------------------------------------
// zipkin_module_ctx
//------------------------------------------------------------------------------
static ngx_http_module_t zipkin_module_ctx = {
    nullptr,                 /* preconfiguration */
    nullptr,                 /* postconfiguration */
    create_zipkin_main_conf, /* create main configuration */
    nullptr,                 /* init main configuration */
    nullptr,                 /* create server configuration */
    nullptr,                 /* merge server configuration */
    nullptr,                 /* create location configuration */
    nullptr                  /* merge location configuration */
};

//------------------------------------------------------------------------------
// zipkin_commands
//------------------------------------------------------------------------------
static ngx_command_t zipkin_commands[] = {
    {ngx_string("zipkin_service_name"), NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
     ngx_conf_set_str_slot, NGX_HTTP_MAIN_CONF_OFFSET,
     offsetof(zipkin_main_conf_t, service_name), nullptr},
    {ngx_string("zipkin_collector_host"), NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
     ngx_conf_set_str_slot, NGX_HTTP_MAIN_CONF_OFFSET,
     offsetof(zipkin_main_conf_t, collector_host), nullptr},
    {ngx_string("zipkin_collector_port"), NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
     ngx_conf_set_str_slot, NGX_HTTP_MAIN_CONF_OFFSET,
     offsetof(zipkin_main_conf_t, collector_port), nullptr},
    {ngx_string("zipkin_sample_rate"), NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
     ngx_conf_set_str_slot, NGX_HTTP_MAIN_CONF_OFFSET,
     offsetof(zipkin_main_conf_t, sample_rate), nullptr}};

//------------------------------------------------------------------------------
// ngx_http_zipkin_module
//------------------------------------------------------------------------------
ngx_module_t ngx_http_zipkin_module = {NGX_MODULE_V1,
                                       &zipkin_module_ctx, /* module context */
                                       zipkin_commands, /* module directives */
                                       NGX_HTTP_MODULE, /* module type */
                                       nullptr,         /* init master */
                                       nullptr,         /* init module */
                                       zipkin_init_worker, /* init process */
                                       nullptr,            /* init thread */
                                       nullptr,            /* exit thread */
                                       nullptr,            /* exit process */
                                       nullptr,            /* exit master */
                                       NGX_MODULE_V1_PADDING};
