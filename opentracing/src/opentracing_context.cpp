#include "opentracing_context.h"
#include "utility.h"

#include <sstream>
#include <stdexcept>

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
std::unique_ptr<opentracing::SpanContext> extract_span_context(
    const opentracing::Tracer &tracer, const ngx_http_request_t *request);

//------------------------------------------------------------------------------
// OpenTracingContext
//------------------------------------------------------------------------------
OpenTracingContext::OpenTracingContext(ngx_http_request_t *request,
                                       ngx_http_core_loc_conf_t *core_loc_conf,
                                       opentracing_loc_conf_t *loc_conf) {
  traces_.emplace_back(request, core_loc_conf, loc_conf);
}

//------------------------------------------------------------------------------
// on_change_block
//------------------------------------------------------------------------------
void OpenTracingContext::on_change_block(
    ngx_http_request_t *request, ngx_http_core_loc_conf_t *core_loc_conf,
    opentracing_loc_conf_t *loc_conf) {
  auto trace = find_trace(request);
  if (trace != nullptr) {
    return trace->on_change_block(core_loc_conf, loc_conf);
  }

  // This is a new subrequest so add a RequestTracing for it.
  traces_.emplace_back(request, core_loc_conf, loc_conf, &traces_[0].context());
}

//------------------------------------------------------------------------------
// on_log_request
//------------------------------------------------------------------------------
void OpenTracingContext::on_log_request(ngx_http_request_t *request) {
  auto trace = find_trace(request);
  if (trace == nullptr) {
    throw std::runtime_error{
        "on_log_request failed: could not find request trace"};
  }
  trace->on_log_request();
}

//------------------------------------------------------------------------------
// lookup_span_context_value
//------------------------------------------------------------------------------
ngx_str_t OpenTracingContext::lookup_span_context_value(
    ngx_http_request_t *request, opentracing::string_view key) {
  auto trace = find_trace(request);
  if (trace == nullptr) {
    throw std::runtime_error{
        "lookup_span_context_value failed: could not find request trace"};
  }
  return trace->lookup_span_context_value(key);
}

//------------------------------------------------------------------------------
// get_binary_context
//------------------------------------------------------------------------------
ngx_str_t OpenTracingContext::get_binary_context(
    ngx_http_request_t *request) const {
  auto trace = find_trace(request);
  if (trace == nullptr) {
    throw std::runtime_error{
        "get_binary_context failed: could not find request trace"};
  }
  return trace->get_binary_context();
}

//------------------------------------------------------------------------------
// find_trace
//------------------------------------------------------------------------------
RequestTracing *OpenTracingContext::find_trace(ngx_http_request_t *request) {
  for (auto &trace : traces_) {
    if (trace.request() == request) {
      return &trace;
    }
  }
  return nullptr;
}

const RequestTracing *OpenTracingContext::find_trace(
    ngx_http_request_t *request) const {
  return const_cast<OpenTracingContext *>(this)->find_trace(request);
}

//------------------------------------------------------------------------------
// cleanup_opentracing_context
//------------------------------------------------------------------------------
static void cleanup_opentracing_context(void *data) noexcept {
  delete static_cast<OpenTracingContext *>(data);
}

//------------------------------------------------------------------------------
// find_opentracing_cleanup
//------------------------------------------------------------------------------
static ngx_pool_cleanup_t *find_opentracing_cleanup(
    ngx_http_request_t *request) {
  for (auto cleanup = request->pool->cleanup; cleanup;
       cleanup = cleanup->next) {
    if (cleanup->handler == cleanup_opentracing_context) {
      return cleanup;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
// get_opentracing_context
//------------------------------------------------------------------------------
OpenTracingContext *get_opentracing_context(
    ngx_http_request_t *request) noexcept {
  auto context = static_cast<OpenTracingContext *>(
      ngx_http_get_module_ctx(request, ngx_http_opentracing_module));
  if (context != nullptr || !request->internal) {
    return context;
  }

  // If this is an internal redirect, the OpenTracingContext will have been
  // reset, but we can still recover it from the cleanup handler.
  //
  // See set_opentracing_context below.
  auto cleanup = find_opentracing_cleanup(request);
  if (cleanup != nullptr) {
    context = static_cast<OpenTracingContext *>(cleanup->data);
  }

  // If we found a context, attach with ngx_http_set_ctx so that we don't have
  // to loop through the cleanup handlers again.
  if (context != nullptr) {
    ngx_http_set_ctx(request, static_cast<void *>(context),
                     ngx_http_opentracing_module);
  }

  return context;
}

//------------------------------------------------------------------------------
// set_opentracing_context
//------------------------------------------------------------------------------
// Attaches an OpenTracingContext to a request.
//
// Note that internal redirects for nginx will clear any data attached via
// ngx_http_set_ctx. Since OpenTracingContext needs to persist across
// redirection, as a workaround the context is stored in a cleanup handler where
// it can be later recovered.
//
// See the discussion in
//    https://forum.nginx.org/read.php?29,272403,272403#msg-272403
// or the approach taken by the standard nginx realip module.
void set_opentracing_context(ngx_http_request_t *request,
                             OpenTracingContext *context) {
  auto cleanup = ngx_pool_cleanup_add(request->pool, 0);
  if (cleanup == nullptr) {
    delete context;
    throw std::runtime_error{"failed to allocate cleanup handler"};
  }
  cleanup->data = static_cast<void *>(context);
  cleanup->handler = cleanup_opentracing_context;
  ngx_http_set_ctx(request, static_cast<void *>(context),
                   ngx_http_opentracing_module);
}

//------------------------------------------------------------------------------
// destroy_opentracing_context
//------------------------------------------------------------------------------
// Supports early destruction of the OpenTracingContext (in case of an
// unrecoverable error).
void destroy_opentracing_context(ngx_http_request_t *request) noexcept {
  auto cleanup = find_opentracing_cleanup(request);
  if (cleanup == nullptr) {
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                  "Unable to find OpenTracing cleanup handler for request %p",
                  request);
    return;
  }
  delete static_cast<OpenTracingContext *>(cleanup->data);
  cleanup->data = nullptr;
  ngx_http_set_ctx(request, nullptr, ngx_http_opentracing_module);
}
}  // namespace ngx_opentracing
