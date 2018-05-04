#include "discover_span_context_keys.h"
#include "load_tracer.h"

#include <opentracing/propagation.h>
#include <opentracing/ext/tags.h>
#include <iostream>

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// HeaderKeyWriter
//------------------------------------------------------------------------------
namespace {
class HeaderKeyWriter : public opentracing::HTTPHeadersWriter {
 public:
  opentracing::expected<void> Set(
      opentracing::string_view key,
      opentracing::string_view value) const override {
    std::cerr << "key = " << key << std::endl;
    return {};
  }
};
}

//------------------------------------------------------------------------------
// discover_span_context_keys
//------------------------------------------------------------------------------
ngx_int_t discover_span_context_keys(ngx_log_t* log, const char* tracing_library,
                                const char* tracer_config_file) {
  opentracing::DynamicTracingLibraryHandle handle;
  std::shared_ptr<opentracing::Tracer> tracer;
  auto result =
      load_tracer(log, tracing_library, tracer_config_file, handle, tracer);
  if (result != NGX_OK) {
    return result;
  }
  auto span = tracer->StartSpan("dummySpan");
  auto carrier_writer = HeaderKeyWriter{};
  auto was_successful = tracer->Inject(span->context(), carrier_writer);
  span->SetTag(opentracing::ext::sampling_priority, 0);
  if (!was_successful) {
    ngx_log_error(NGX_LOG_ERR, log, 0,
                  "Failed to discover span context tags: %s",
                  was_successful.error().message().c_str());
    return NGX_ERROR;
  }
  return NGX_OK;
}
} // namespace ngx_opentracing
