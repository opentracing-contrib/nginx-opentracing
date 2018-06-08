#include "load_tracer.h"

#include <opentracing/dynamic_load.h>
#include <cerrno>
#include <fstream>

namespace ngx_opentracing {
ngx_int_t load_tracer(ngx_log_t* log, const char* tracer_library,
                      const char* config_file,
                      opentracing::DynamicTracingLibraryHandle& handle,
                      std::shared_ptr<opentracing::Tracer>& tracer) {
  std::string error_message;

  // Open the library handle
  auto handle_maybe =
      opentracing::DynamicallyLoadTracingLibrary(tracer_library, error_message);
  if (!handle_maybe) {
    if (!error_message.empty()) {
      ngx_log_error(NGX_LOG_ERR, log, 0,
                    "Failed to load tracing library %s: %s", tracer_library,
                    error_message.c_str());
    } else {
      ngx_log_error(NGX_LOG_ERR, log, 0,
                    "Failed to load tracing library %s: %s", tracer_library);
    }
    return NGX_ERROR;
  }
  auto& tracer_factory = handle_maybe->tracer_factory();

  // Construct a tracer
  errno = 0;
  std::ifstream in{config_file};
  if (!in.good()) {
    ngx_log_error(NGX_LOG_ERR, log, errno,
                  "Failed to open tracer configuration file %s", config_file);
    return NGX_ERROR;
  }
  std::string tracer_config{std::istreambuf_iterator<char>{in},
                            std::istreambuf_iterator<char>{}};
  if (!in.good()) {
    ngx_log_error(NGX_LOG_ERR, log, errno,
                  "Failed to read tracer configuration file %s", &config_file);
    return NGX_ERROR;
  }

  auto tracer_maybe =
      tracer_factory.MakeTracer(tracer_config.c_str(), error_message);
  if (!tracer_maybe) {
    if (!error_message.empty()) {
      ngx_log_error(NGX_LOG_ERR, log, 0, "Failed to construct tracer: %s",
                    error_message.c_str());
    } else {
      ngx_log_error(NGX_LOG_ERR, log, 0, "Failed to construct tracer: %s",
                    tracer_maybe.error().message().c_str());
    }
    return NGX_ERROR;
  }

  handle = std::move(*handle_maybe);
  tracer = std::move(*tracer_maybe);

  return NGX_OK;
}
}  // namespace ngx_opentracing
