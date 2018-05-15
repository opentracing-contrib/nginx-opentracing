#include "span_context_querier.h"

#include "utility.h"

#include <opentracing/propagation.h>
#include <opentracing/tracer.h>

#include <algorithm>
#include <cassert>
#include <new>

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// constructor
//------------------------------------------------------------------------------
SpanContextQuerier::SpanContextQuerier(
    const opentracing_main_conf_t& conf) noexcept
    : num_keys_{static_cast<int>(conf.span_context_keys->nelts)},
      keys_{static_cast<opentracing::string_view*>(
          conf.span_context_keys->elts)} {}

//------------------------------------------------------------------------------
// lookup_value
//------------------------------------------------------------------------------
ngx_str_t SpanContextQuerier::lookup_value(ngx_http_request_t* request,
                                           const opentracing::Span& span,
                                           int value_index) {
  assert(value_index < num_keys_);
  if (&span != values_span_) {
    expand_span_context_values(request, span);
  }
  auto value = values_[value_index];
  return to_ngx_str(value);
}

//------------------------------------------------------------------------------
// SpanContextValueExpander
//------------------------------------------------------------------------------
namespace {
class SpanContextValueExpander : public opentracing::HTTPHeadersWriter {
 public:
  SpanContextValueExpander(ngx_http_request_t* request, int num_keys,
                           opentracing::string_view* keys,
                           opentracing::string_view* values)
      : request_{request}, num_keys_{num_keys}, keys_{keys}, values_{values} {}

  opentracing::expected<void> Set(
      opentracing::string_view key,
      opentracing::string_view value) const override {
    for (int index = 0; index < num_keys_; ++index) {
      if (keys_[index] == key) {
        auto data =
            static_cast<char*>(ngx_pnalloc(request_->pool, value.size()));
        if (data == nullptr) {
          throw std::bad_alloc{};
        }
        std::copy_n(value.data(), value.size(), data);
        values_[index] = opentracing::string_view{data, value.size()};
        return {};
      }
    }

    // If we got here, then the tracer is using a key for propagation that
    // wasn't discovered at initialization. (See set_tracer). It won't be
    // propagated so log an error.
    ngx_log_error(NGX_LOG_ERR, request_->connection->log, 0,
                  "dropping span context key %V for request %p",
                  to_ngx_str(key), request_);

    return {};
  }

 private:
  ngx_http_request_t* request_;
  int num_keys_;
  opentracing::string_view* keys_;
  opentracing::string_view* values_;
};
}  // namespace

//------------------------------------------------------------------------------
// expand_span_context_values
//------------------------------------------------------------------------------
void SpanContextQuerier::expand_span_context_values(
    ngx_http_request_t* request, const opentracing::Span& span) {
  values_span_ = &span;
  values_ = static_cast<opentracing::string_view*>(
      ngx_pcalloc(request->pool, sizeof(opentracing::string_view) * num_keys_));
  if (values_ == nullptr) {
    throw std::bad_alloc{};
  }
  SpanContextValueExpander carrier{request, num_keys_, keys_, values_};
  auto was_successful = span.tracer().Inject(span.context(), carrier);
  if (!was_successful) {
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                  "Tracer.inject() failed for request %p: %s", request,
                  was_successful.error().message().c_str());
  }
}
}  // namespace ngx_opentracing
