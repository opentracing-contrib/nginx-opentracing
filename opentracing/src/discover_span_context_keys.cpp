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
  HeaderKeyWriter(ngx_pool_t* pool, std::vector<opentracing::string_view>& keys)
      : pool_{pool}, keys_{keys} {}

  opentracing::expected<void> Set(
      opentracing::string_view key,
      opentracing::string_view value) const override {
    auto data =
        static_cast<char*>(ngx_palloc(pool_, key.size()));
    if (data == nullptr) {
      return opentracing::make_unexpected(
          std::make_error_code(std::errc::not_enough_memory));
    }
    std::copy_n(key.data(), key.size(), data);

    keys_.emplace_back(data, key.size());

    return {};
  }
 private:
  ngx_pool_t* pool_;
  std::vector<opentracing::string_view>& keys_;
};
}

//------------------------------------------------------------------------------
// discover_span_context_keys
//------------------------------------------------------------------------------
ngx_array_t* discover_span_context_keys(ngx_pool_t* pool, ngx_log_t* log,
                                        const char* tracing_library,
                                        const char* tracer_config_file) {
  opentracing::DynamicTracingLibraryHandle handle;
  std::shared_ptr<opentracing::Tracer> tracer;
  auto rcode =
      load_tracer(log, tracing_library, tracer_config_file, handle, tracer);
  if (rcode != NGX_OK) {
    return nullptr;
  }
  auto span = tracer->StartSpan("dummySpan");
  std::vector<opentracing::string_view> keys;
  HeaderKeyWriter carrier_writer{pool, keys};
  /* auto carrier_writer = HeaderKeyWriter{span_context_keys}; */
  auto was_successful = tracer->Inject(span->context(), carrier_writer);
  span->SetTag(opentracing::ext::sampling_priority, 0);
  if (!was_successful) {
    ngx_log_error(NGX_LOG_ERR, log, 0,
                  "Failed to discover span context tags: %s",
                  was_successful.error().message().c_str());
    return nullptr;
  }
  ngx_array_t* result =
      ngx_array_create(pool, keys.size(), sizeof(opentracing::string_view));
  if (result == nullptr) {
    return result;
  }

  for (auto key : keys) {
    auto element = static_cast<opentracing::string_view*>(ngx_array_push(result));
    *element = key;
  }
  return result;
}
} // namespace ngx_opentracing
