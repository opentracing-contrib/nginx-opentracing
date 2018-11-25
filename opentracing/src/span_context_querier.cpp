#include "span_context_querier.h"

#include "utility.h"

#include <opentracing/propagation.h>
#include <opentracing/tracer.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <new>

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// lookup_value
//------------------------------------------------------------------------------
ngx_str_t SpanContextQuerier::lookup_value(ngx_http_request_t* request,
                                           const opentracing::Span& span,
                                           opentracing::string_view key) {
  if (&span != values_span_) {
    expand_span_context_values(request, span);
  }

  for (auto& key_value : span_context_expansion_) {
    if (key_value.first == key) {
      return to_ngx_str(key_value.second);
    }
  }

  auto ngx_key = to_ngx_str(key);
  ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                "no opentracing context value found for span context key %V "
                "for request %p",
                &ngx_key, request);
  return ngx_str_t();
}

//------------------------------------------------------------------------------
// SpanContextValueExpander
//------------------------------------------------------------------------------
namespace {
class SpanContextValueExpander : public opentracing::HTTPHeadersWriter {
 public:
  explicit SpanContextValueExpander(
      std::vector<std::pair<std::string, std::string>>& span_context_expansion)
      : span_context_expansion_(span_context_expansion) {}

  opentracing::expected<void> Set(
      opentracing::string_view key,
      opentracing::string_view value) const override {
    std::string key_copy;
    key_copy.reserve(key.size());
    std::transform(std::begin(key), std::end(key), std::back_inserter(key_copy),
                   header_transform);

    span_context_expansion_.emplace_back(std::move(key_copy), value);
    return {};
  }

 private:
  std::vector<std::pair<std::string, std::string>>& span_context_expansion_;
};
}  // namespace

//------------------------------------------------------------------------------
// expand_span_context_values
//------------------------------------------------------------------------------
void SpanContextQuerier::expand_span_context_values(
    ngx_http_request_t* request, const opentracing::Span& span) {
  values_span_ = &span;
  span_context_expansion_.clear();
  SpanContextValueExpander carrier{span_context_expansion_};
  auto was_successful = span.tracer().Inject(span.context(), carrier);
  if (!was_successful) {
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                  "Tracer.inject() failed for request %p: %s", request,
                  was_successful.error().message().c_str());
  }
}
}  // namespace ngx_opentracing
