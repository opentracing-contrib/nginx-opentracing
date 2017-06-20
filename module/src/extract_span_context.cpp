/* #include <lightstep/impl.h> */
/* #include <lightstep/tracer.h> */
/* #include <ngx_opentracing_utility.h> */

extern "C" {
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

/* namespace ngx_opentracing { */
/* //------------------------------------------------------------------------------
 */
/* // NgxHeaderCarrierReader */
/* //------------------------------------------------------------------------------
 */
/* namespace { */
/* class NgxHeaderCarrierReader : public lightstep::BasicCarrierReader { */
/*  public: */
/*   explicit NgxHeaderCarrierReader(const ngx_http_request_t *request) */
/*       : request_{request} {} */

/*   void ForeachKey( */
/*       std::function<void(const std::string &, const std::string &value)> f)
 */
/*       const { */
/*     std::string key, value; */
/*     for_each<ngx_table_elt_t>( */
/*         request_->headers_in.headers, [&](const ngx_table_elt_t &header) { */
/*           key.assign(reinterpret_cast<char *>(header.lowcase_key), */
/*                      header.key.len); */
/*           value.assign(reinterpret_cast<char *>(header.value.data), */
/*                        header.value.len); */
/*           f(key, value); */
/*         }); */
/*   } */

/*  private: */
/*   const ngx_http_request_t *request_; */
/* }; */
/* } */

/* //------------------------------------------------------------------------------
 */
/* // extract_span_context */
/* //------------------------------------------------------------------------------
 */
/* lightstep::SpanContext extract_span_context(lightstep::Tracer &tracer, */
/*                                             const ngx_http_request_t
 * *request) { */
/*   auto carrier_reader = NgxHeaderCarrierReader{request}; */
/*   auto span_context = */
/*       tracer.Extract(lightstep::CarrierFormat::HTTPHeaders, carrier_reader);
 */
/*   if (span_context.valid()) { */
/*     ngx_log_debug3(NGX_LOG_DEBUG_HTTP, request->connection->log, 0, */
/*                    "extraced opentracing span context (trace_id=%uxL" */
/*                    ", span_id=%uxL) from request %p", */
/*                    span_context.trace_id(), span_context.span_id(), request);
 */
/*   } else { */
/*     ngx_log_debug1( */
/*         NGX_LOG_DEBUG_HTTP, request->connection->log, 0, */
/*         "failed to extract an opentracing span context from request %p", */
/*         request); */
/*   } */
/*   return span_context; */
/* } */
/* }  // namespace ngx_opentracing */
