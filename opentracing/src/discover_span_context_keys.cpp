#include "discover_span_context_keys.h"
#include "load_tracer.h"

#include <opentracing/propagation.h>
#include <opentracing/ext/tags.h>
#include <iostream>
#include <algorithm>

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// HeaderKeyWriter
//------------------------------------------------------------------------------
namespace {
class HeaderKeyWriter : public opentracing::HTTPHeadersWriter {
 public:
  explicit HeaderKeyWriter(ngx_array_t* span_context_keys)
      : span_context_keys_{span_context_keys} {}

  opentracing::expected<void> Set(
      opentracing::string_view key,
      opentracing::string_view value) const override {
    auto data =
        static_cast<char*>(ngx_palloc(span_context_keys_->pool, key.size()));
    if (data == nullptr) {
      return opentracing::make_unexpected(
          std::make_error_code(std::errc::not_enough_memory));
    }
    std::copy_n(key.data(), key.size(), data);

    auto element = static_cast<opentracing::string_view*>(
        ngx_array_push(span_context_keys_));
    if (element == nullptr) {
      return opentracing::make_unexpected(
          std::make_error_code(std::errc::not_enough_memory));
    }
    *element = {data, key.size()};

    return {};
  }
 private:
  ngx_array_t* span_context_keys_;
};
}

//------------------------------------------------------------------------------
// discover_span_context_keys
//------------------------------------------------------------------------------
ngx_int_t discover_span_context_keys(ngx_log_t* log,
                                     const char* tracing_library,
                                     const char* tracer_config_file,
                                     ngx_array_t* span_context_keys) {
  opentracing::DynamicTracingLibraryHandle handle;
  std::shared_ptr<opentracing::Tracer> tracer;
  auto result =
      load_tracer(log, tracing_library, tracer_config_file, handle, tracer);
  if (result != NGX_OK) {
    return result;
  }
  auto span = tracer->StartSpan("dummySpan");
  auto carrier_writer = HeaderKeyWriter{span_context_keys};
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
