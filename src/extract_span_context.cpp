#include <lightstep/impl.h>
#include <lightstep/tracer.h>
#include <ngx_opentracing_utility.h>

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// NgxHeaderCarrierReader
//------------------------------------------------------------------------------
namespace {
class NgxHeaderCarrierReader : public lightstep::BasicCarrierReader {
public:
  explicit NgxHeaderCarrierReader(const ngx_http_request_t *request)
      : request_{request} {}

  void ForeachKey(
      std::function<void(const std::string &, const std::string &value)> f)
      const {
    std::string key, value;
    for_each<ngx_table_elt_t>(
        request_->headers_in.headers, [&](const ngx_table_elt_t &header) {
          key.assign(reinterpret_cast<char *>(header.lowcase_key),
                     header.key.len);
          value.assign(reinterpret_cast<char *>(header.value.data),
                       header.value.len);
          f(key, value);
        });
  }

private:
  const ngx_http_request_t *request_;
};
}

//------------------------------------------------------------------------------
// extract_span_context
//------------------------------------------------------------------------------
lightstep::SpanContext extract_span_context(lightstep::Tracer &tracer,
                                            const ngx_http_request_t *request) {
  auto carrier_reader = NgxHeaderCarrierReader{request};
  return tracer.Extract(lightstep::CarrierFormat::HTTPHeaders, carrier_reader);
}
} // namespace ngx_opentracing
