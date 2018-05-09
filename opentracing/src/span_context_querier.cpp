#include "span_context_querier.h"

#include <opentracing/propagation.h>

#include <cassert>

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// constructor
//------------------------------------------------------------------------------
SpanContextQuerier::SpanContextQuerier(const opentracing_main_conf_t& conf)
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
  return {value.size(),
          reinterpret_cast<unsigned char*>(const_cast<char*>(value.data()))};
}

//------------------------------------------------------------------------------
// SpanContextValueExpander
//------------------------------------------------------------------------------
namespace {
class SpanContextValueExpander : public opentracing::HTTPHeadersWriter {
 public:
  opentracing::expected<void> Set(
      opentracing::string_view key,
      opentracing::string_view value) const override {
    return {};
  }
 private:
};
}

//------------------------------------------------------------------------------
// expand_span_context_values
//------------------------------------------------------------------------------
void SpanContextQuerier::expand_span_context_values(
    ngx_http_request_t* request, const opentracing::Span& span) {
  values_span_ = &span;
  (void)keys_;
}
}  // namespace ngx_opentracing
