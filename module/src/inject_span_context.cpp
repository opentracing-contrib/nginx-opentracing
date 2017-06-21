#include <ngx_opentracing_utility.h>
#include <opentracing/propagation.h>
#include <opentracing/tracer.h>
#include <system_error>
using opentracing::Expected;
using opentracing::make_unexpected;

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// insert_header
//------------------------------------------------------------------------------
static Expected<void> insert_header(ngx_http_request_t *request, ngx_str_t key,
                                    ngx_str_t value) {
  auto header = static_cast<ngx_table_elt_t *>(
      ngx_list_push(&request->headers_in.headers));
  if (!header)
    return make_unexpected(std::make_error_code(std::errc::not_enough_memory));
  header->hash = 1;
  header->key = key;
  header->lowcase_key = key.data;
  header->value = value;
  return {};
}

//------------------------------------------------------------------------------
// set_headers
//------------------------------------------------------------------------------
static Expected<void> set_headers(
    ngx_http_request_t *request,
    std::vector<std::pair<ngx_str_t, ngx_str_t>> &headers) {
  if (headers.empty()) return {};

  // If header keys are already in the request, overwrite the values instead of
  // inserting a new header.
  //
  // It may be possible in some cases to use nginx's hashes to look up the
  // entries faster, but then we'd have to handle the special case of when a
  // header element isn't hashed yet. Iterating over the header entries all the
  // time keeps things simple.
  for_each<ngx_table_elt_t>(
      request->headers_in.headers, [&](ngx_table_elt_t &header) {
        auto i = std::find_if(
            headers.begin(), headers.end(),
            [&](const std::pair<ngx_str_t, ngx_str_t> &key_value) {
              const auto &key = key_value.first;
              return header.key.len == key.len &&
                     ngx_strncmp(reinterpret_cast<char *>(header.lowcase_key),
                                 reinterpret_cast<char *>(key.data),
                                 key.len) == 0;

            });
        if (i == headers.end()) return;
        ngx_log_debug4(
            NGX_LOG_DEBUG_HTTP, request->connection->log, 0,
            "replacing opentracing header \"%V:%V\" with value \"%V\""
            " in request %p",
            &header.key, &header.value, &i->second, request);
        header.value = i->second;
        headers.erase(i);
      });

  // Any header left in `headers` doesn't already have a key in the request, so
  // create a new entry for it.
  for (const auto &key_value : headers) {
    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, request->connection->log, 0,
                   "adding opentracing header \"%V:%V\" in request %p",
                   &key_value.first, &key_value.second, request);
    auto was_successful =
        insert_header(request, key_value.first, key_value.second);
    if (!was_successful) {
      ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                    "failed to insert header");
      return was_successful;
    }
  }
  return {};
}

//------------------------------------------------------------------------------
// NgxHeaderCarrierWriter
//------------------------------------------------------------------------------
namespace {
class NgxHeaderCarrierWriter : public opentracing::HTTPHeadersWriter {
 public:
  NgxHeaderCarrierWriter(ngx_http_request_t *request,
                         std::vector<std::pair<ngx_str_t, ngx_str_t>> &headers)
      : request_{request}, headers_{headers} {}

  Expected<void> Set(const std::string &key,
                     const std::string &value) const override {
    auto ngx_key = to_lower_ngx_str(request_->pool, key);
    if (!ngx_key.data) {
      ngx_log_error(NGX_LOG_ERR, request_->connection->log, 0,
                    "failed to allocate header key");
      return make_unexpected(
          std::make_error_code(std::errc::not_enough_memory));
    }
    auto ngx_value = to_ngx_str(request_->pool, value);
    if (!ngx_value.data) {
      ngx_log_error(NGX_LOG_ERR, request_->connection->log, 0,
                    "failed to allocate header value");
      return make_unexpected(
          std::make_error_code(std::errc::not_enough_memory));
    }
    headers_.emplace_back(ngx_key, ngx_value);
    return {};
  }

 private:
  ngx_http_request_t *request_;
  std::vector<std::pair<ngx_str_t, ngx_str_t>> &headers_;
};
}

//------------------------------------------------------------------------------
// inject_span_context
//------------------------------------------------------------------------------
void inject_span_context(const opentracing::Tracer &tracer,
                         ngx_http_request_t *request,
                         const opentracing::SpanContext &span_context) {
  ngx_log_debug1(NGX_LOG_DEBUG_HTTP, request->connection->log, 0,
                 "injecting opentracing span context from request %p", request);
  std::vector<std::pair<ngx_str_t, ngx_str_t>> headers;
  auto carrier_writer = NgxHeaderCarrierWriter{request, headers};
  auto was_successful = tracer.Inject(
      span_context, opentracing::CarrierFormat::HTTPHeaders, carrier_writer);
  if (was_successful) was_successful = set_headers(request, headers);
  if (!was_successful)
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                  "Tracer.inject() failed for request %p: %s", request,
                  was_successful.error().message().c_str());
}
}  // namespace ngx_opentracing
