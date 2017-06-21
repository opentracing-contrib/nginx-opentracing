#include "opentracing_request_instrumentor.h"
#include <ngx_opentracing_utility.h>
#include <iostream>

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
std::unique_ptr<opentracing::SpanContext> extract_span_context(
    const opentracing::Tracer &tracer, const ngx_http_request_t *request);

void inject_span_context(const opentracing::Tracer &tracer,
                         ngx_http_request_t *request,
                         const opentracing::SpanContext &span_context);

//------------------------------------------------------------------------------
// get_loc_operation_name
//------------------------------------------------------------------------------
static std::string get_loc_operation_name(
    ngx_http_request_t *request, const ngx_http_core_loc_conf_t *core_loc_conf,
    const opentracing_loc_conf_t *loc_conf) {
  if (loc_conf->loc_operation_name_script.is_valid())
    return to_string(loc_conf->loc_operation_name_script.run(request));
  else
    return to_string(core_loc_conf->name);
}

//------------------------------------------------------------------------------
// get_request_operation_name
//------------------------------------------------------------------------------
static std::string get_request_operation_name(
    ngx_http_request_t *request, const ngx_http_core_loc_conf_t *core_loc_conf,
    const opentracing_loc_conf_t *loc_conf) {
  if (loc_conf->operation_name_script.is_valid())
    return to_string(loc_conf->operation_name_script.run(request));
  else
    return to_string(core_loc_conf->name);
}

//------------------------------------------------------------------------------
// add_script_tags
//------------------------------------------------------------------------------
static void add_script_tags(ngx_array_t *tags, ngx_http_request_t *request,
                            opentracing::Span &span) {
  if (!tags) return;
  auto add_tag = [&](const opentracing_tag_t &tag) {
    auto key = tag.key_script.run(request);
    auto value = tag.value_script.run(request);
    if (key.data && value.data) span.SetTag(to_string(key), to_string(value));
  };
  for_each<opentracing_tag_t>(*tags, add_tag);
}

//------------------------------------------------------------------------------
// add_status_tags
//------------------------------------------------------------------------------
static void add_status_tags(const ngx_http_request_t *request,
                            opentracing::Span &span) {
  // Check for errors.
  // TODO: Should we also look at request->err_status?
  auto status = request->headers_out.status;
  auto status_line = to_string(request->headers_out.status_line);
  if (status != 0) span.SetTag("http.status_code", status);
  if (status_line.data()) span.SetTag("http.status_line", status_line);
  // Treat any 5xx code as an error.
  if (status >= 500) {
    span.SetTag("error", true);
    span.Log({{"event", "error"}, {"message", status_line}});
  }
}

//------------------------------------------------------------------------------
// OpenTracingRequestInstrumentor
//------------------------------------------------------------------------------
OpenTracingRequestInstrumentor::OpenTracingRequestInstrumentor(
    ngx_http_request_t *request, ngx_http_core_loc_conf_t *core_loc_conf,
    opentracing_loc_conf_t *loc_conf)
    : request_{request}, loc_conf_{loc_conf} {
  main_conf_ = static_cast<opentracing_main_conf_t *>(
      ngx_http_get_module_main_conf(request_, ngx_http_opentracing_module));
  auto tracer = opentracing::Tracer::Global();
  if (!tracer) throw InstrumentationFailure{};
  auto parent_span_context = extract_span_context(*tracer, request_);

  ngx_log_debug1(NGX_LOG_DEBUG_HTTP, request_->connection->log, 0,
                 "starting opentracing request span for %p", request_);
  request_span_ = tracer->StartSpan(
      get_request_operation_name(request_, core_loc_conf, loc_conf_),
      {opentracing::ChildOf(parent_span_context.get())});
  if (!request_span_) throw InstrumentationFailure{};

  if (loc_conf_->enable_locations) {
    ngx_log_debug3(
        NGX_LOG_DEBUG_HTTP, request_->connection->log, 0,
        "starting opentracing location span for \"%V\"(%p) in request %p",
        &core_loc_conf->name, loc_conf_, request_);
    span_ = tracer->StartSpan(
        get_loc_operation_name(request_, core_loc_conf, loc_conf_),
        {opentracing::ChildOf(&request_span_->context())});
    if (!span_) throw InstrumentationFailure{};
  }

  set_request_span_context();
}

//------------------------------------------------------------------------------
// on_change_block
//------------------------------------------------------------------------------
void OpenTracingRequestInstrumentor::on_change_block(
    ngx_http_core_loc_conf_t *core_loc_conf, opentracing_loc_conf_t *loc_conf) {
  on_exit_block();
  auto prev_loc_conf = loc_conf_;
  loc_conf_ = loc_conf;

  if (loc_conf->enable_locations) {
    ngx_log_debug3(
        NGX_LOG_DEBUG_HTTP, request_->connection->log, 0,
        "starting opentracing location span for \"%V\"(%p) in request %p",
        &core_loc_conf->name, loc_conf_, request_);
    span_ = request_span_->tracer().StartSpan(
        get_loc_operation_name(request_, core_loc_conf, loc_conf),
        {opentracing::ChildOf(&request_span_->context())});
    if (!span_) throw InstrumentationFailure{};
  }

  // As an optimization, avoid injecting the request span context if neither the
  // previous nor current location blocks are traced since the active span
  // context will be the same.
  if (prev_loc_conf->enable_locations || loc_conf->enable_locations)
    set_request_span_context();
}

//------------------------------------------------------------------------------
// on_exit_block
//------------------------------------------------------------------------------
void OpenTracingRequestInstrumentor::on_exit_block() {
  // Set default and custom tags for the block. Many nginx variables won't be
  // available when a block is first entered, so set tags when the block is
  // exited instead.
  if (loc_conf_->enable_locations) {
    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, request_->connection->log, 0,
                   "finishing opentracing location span for %p in request %p",
                   loc_conf_, request_);
    add_script_tags(main_conf_->tags, request_, *span_);
    add_script_tags(loc_conf_->tags, request_, *span_);
    add_status_tags(request_, *span_);
    span_->Finish();
  } else {
    add_script_tags(loc_conf_->tags, request_, *request_span_);
  }
}

//------------------------------------------------------------------------------
// set_request_span_context
//------------------------------------------------------------------------------
void OpenTracingRequestInstrumentor::set_request_span_context() {
  if (loc_conf_->enable_locations)
    inject_span_context(span_->tracer(), request_, span_->context());
  else
    inject_span_context(request_span_->tracer(), request_,
                        request_span_->context());
}

//------------------------------------------------------------------------------
// on_log_request
//------------------------------------------------------------------------------
void OpenTracingRequestInstrumentor::on_log_request() {
  on_exit_block();

  ngx_log_debug1(NGX_LOG_DEBUG_HTTP, request_->connection->log, 0,
                 "finishing opentracing request span for %p", request_);
  add_status_tags(request_, *request_span_);
  add_script_tags(main_conf_->tags, request_, *request_span_);

  request_span_->Finish();
}
}  // namespace ngx_opentracing
