extern "C" {

#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

}  // extern "C"

#include <jaegertracing/Tracer.h>

extern ngx_module_t ngx_http_jaeger_module;

namespace {

struct Config {
    ngx_str_t serviceName;
    ngx_flag_t disabled;
    // samplers::Config
    ngx_str_t samplerType;
    ngx_str_t samplerParam;
    ngx_str_t samplingServerURL;
    ngx_uint_t samplerMaxOperations;
    ngx_uint_t samplingRefreshIntervalSeconds;
    // reporters::Config
    ngx_uint_t reporterQueueSize;
    ngx_uint_t reporterBufferFlushIntervalSeconds;
    ngx_flag_t reporterLogSpans;
    ngx_str_t reporterLocalAgentHostPort;
    // propagation::HeadersConfig
    ngx_str_t jaegerDebugHeader;
    ngx_str_t jaegerBaggageHeader;
    ngx_str_t traceContextHeaderName;
    ngx_str_t traceBaggageHeaderPrefix;
};

inline std::string makeStr(const ngx_str_t& str)
{
    return std::string(str.data, str.data + str.len);
}

inline double makeDouble(const ngx_str_t& str)
{
    std::istringstream iss(makeStr(str));
    double result = 0;
    if (iss >> result) {
        return result;
    }
    return 0;
}

ngx_int_t moduleInit(ngx_conf_t* cf)
{
    auto* rawConfig = static_cast<Config*>(
        ngx_http_conf_get_module_main_conf(cf, ngx_http_jaeger_module));
    if (rawConfig->serviceName.len <= 0) {
        ngx_log_error(NGX_LOG_ERR,
                      cf->log,
                      0,
                      "`jaeger_service_name` must be specified");
        return NGX_ERROR;
    }
    return NGX_OK;
}

ngx_int_t initWorker(ngx_cycle_t* cycle)
{
    auto* rawConfig = static_cast<Config*>(
        ngx_http_cycle_get_module_main_conf(cycle, ngx_http_jaeger_module));
    try {
        const jaegertracing::Config config(
            rawConfig->disabled,
            jaegertracing::samplers::Config(
                makeStr(rawConfig->samplerType),
                makeDouble(rawConfig->samplerParam),
                makeStr(rawConfig->samplingServerURL),
                rawConfig->samplerMaxOperations,
                std::chrono::seconds(
                    rawConfig->samplingRefreshIntervalSeconds)),
            jaegertracing::reporters::Config(
                rawConfig->reporterQueueSize,
                std::chrono::seconds(
                    rawConfig->reporterBufferFlushIntervalSeconds),
                rawConfig->reporterLogSpans,
                makeStr(rawConfig->reporterLocalAgentHostPort)),
            jaegertracing::propagation::HeadersConfig(
                makeStr(rawConfig->jaegerDebugHeader),
                makeStr(rawConfig->jaegerBaggageHeader),
                makeStr(rawConfig->traceContextHeaderName),
                makeStr(rawConfig->traceBaggageHeaderPrefix)),
            jaegertracing::baggage::RestrictionsConfig());
        auto tracer =
            jaegertracing::Tracer::make(makeStr(rawConfig->serviceName),
                                        config);
        opentracing::Tracer::InitGlobal(tracer);
    } catch (...) {
        ngx_log_error(
            NGX_LOG_ERR, cycle->log, 0, "Failed to create Jaeger tracer");
    }
    return NGX_OK;
}

void* createJaegerMainConfig(ngx_conf_t* conf)
{
    auto* mainConfig = static_cast<Config*>(
        ngx_pcalloc(conf->pool, sizeof(Config)));
    if (!mainConfig) {
        return nullptr;
    }
    // Default initialize members.
    *mainConfig = Config();
    return mainConfig;
}

ngx_http_module_t moduleCtx = {
    nullptr,  // preconfiguration
    &moduleInit,  // postconfiguration
    &createJaegerMainConfig,  // create main configuration
    nullptr,  // init main configuration
    nullptr,  // create server configuration
    nullptr,  // merge server configuration
    nullptr,  // create location configuration
    nullptr  // merge location configuration
};

struct JaegerCommands {
    JaegerCommands()
    {
#define DEFINE_COMMAND(commandName, memberName, ngxType) \
    { \
        ngx_command_t command({ \
            ngx_string(#commandName), \
            NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1, \
            ngx_conf_set_## ngxType ## _slot, \
            NGX_HTTP_MAIN_CONF_OFFSET, \
            offsetof(Config, memberName), \
            nullptr \
        }); \
        _commands.emplace_back(std::move(command)); \
    }

        DEFINE_COMMAND(jaeger_service_name, serviceName, str);
        DEFINE_COMMAND(jaeger_disabled, disabled, flag);
        // samplers::Config
        DEFINE_COMMAND(jaeger_sampler_type, samplerType, str);
        DEFINE_COMMAND(jaeger_sampler_param, samplerParam, str);
        DEFINE_COMMAND(jaeger_sampling_server_url, samplingServerURL, str);
        DEFINE_COMMAND(
            jaeger_sampler_max_operations, samplerMaxOperations, str);
        DEFINE_COMMAND(jaeger_sampling_refresh_interval_seconds,
                       samplingRefreshIntervalSeconds,
                       size);
        // reporters::Config
        DEFINE_COMMAND(jaeger_reporter_queue_size, reporterQueueSize, size);
        DEFINE_COMMAND(jaeger_reporter_buffer_flush_interval_seconds,
                       reporterBufferFlushIntervalSeconds,
                       size)
        DEFINE_COMMAND(jaeger_reporter_log_spans, reporterLogSpans, flag);
        DEFINE_COMMAND(jaeger_reporter_local_agent_host_port,
                       reporterLocalAgentHostPort,
                       str);
        // propagation::HeadersConfig
        DEFINE_COMMAND(jaeger_debug_header, jaegerDebugHeader, str);
        DEFINE_COMMAND(jaeger_baggage_header, jaegerBaggageHeader, str);
        DEFINE_COMMAND(jaeger_trace_context_header_name,
                       traceContextHeaderName,
                       str);
        DEFINE_COMMAND(jaeger_trace_baggage_header_prefix,
                       traceBaggageHeaderPrefix,
                       str);
        _commands.push_back(ngx_null_command);

#undef DEFINE_COMMAND
    }

    std::vector<ngx_command_t> _commands;
};

ngx_command_t* jaegerCommands()
{
    static JaegerCommands commands;
    return &commands._commands[0];
}

#undef DEFINE_COMMAND

}  // anonymous namespace

ngx_module_t ngx_http_jaeger_module = {
    NGX_MODULE_V1,
    &moduleCtx,  // module context
    jaegerCommands(),  // module directives
    NGX_HTTP_MODULE,  // module type
    nullptr,  // init master
    nullptr,  // init module
    &initWorker,  // init process
    nullptr,  // init thread
    nullptr,  // exit thread
    nullptr,  // exit process
    nullptr,  // exit master
    NGX_MODULE_V1_PADDING
};
