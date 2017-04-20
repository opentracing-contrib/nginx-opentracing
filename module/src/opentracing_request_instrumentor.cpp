#include "opentracing_request_instrumentor.h"
#include <ngx_opentracing_utility.h>
#include <iostream>

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
lightstep::SpanContext extract_span_context(lightstep::Tracer &tracer,
                                            const ngx_http_request_t *request);

void inject_span_context(lightstep::Tracer &tracer, ngx_http_request_t *request,
                         const lightstep::SpanContext span_context);

//------------------------------------------------------------------------------
// get_loc_operation_name
//------------------------------------------------------------------------------
static std::string get_loc_operation_name(
    ngx_http_request_t *request, const ngx_http_core_loc_conf_t *core_loc_conf,
    const opentracing_loc_conf_t *loc_conf) {
  if (loc_conf->operation_name_script.is_valid())
    return to_string(loc_conf->operation_name_script.run(request));
  else
    return to_string(core_loc_conf->name);
}

//------------------------------------------------------------------------------
// get_request_operation_name
//------------------------------------------------------------------------------
static std::string get_request_operation_name(
    ngx_http_request_t *request, const ngx_http_core_loc_conf_t *core_loc_conf,
    const opentracing_loc_conf_t *loc_conf) {
  return get_loc_operation_name(request, core_loc_conf, loc_conf);
}

//------------------------------------------------------------------------------
// add_script_tags
//------------------------------------------------------------------------------
static void add_script_tags(ngx_array_t *tags, ngx_http_request_t *request,
                            lightstep::Span &span) {
  if (!tags) return;
  auto add_tag = [&](const opentracing_tag_t &tag) {
    auto key = tag.key_script.run(request);
    auto value = tag.value_script.run(request);
    if (key.data && value.data) span.SetTag(to_string(key), to_string(value));
  };
  for_each<opentracing_tag_t>(*tags, add_tag);
}

//------------------------------------------------------------------------------
// start_span
//------------------------------------------------------------------------------
static lightstep::Span start_span(
    ngx_http_request_t *request, const ngx_http_core_loc_conf_t *core_loc_conf,
    const opentracing_loc_conf_t *loc_conf, lightstep::Tracer &tracer,
    const lightstep::SpanContext &reference_span_context,
    lightstep::SpanReferenceType reference_type) {
  // Start a new span for the location block.
  std::string operation_name;
  if (loc_conf->operation_name_script.is_valid())
    operation_name = to_string(loc_conf->operation_name_script.run(request));
  else
    operation_name = to_string(core_loc_conf->name);
  lightstep::Span span;
  if (reference_span_context.valid())
    span = tracer.StartSpan(
        operation_name,
        {lightstep::SpanReference{reference_type, reference_span_context}});
  else
    span = tracer.StartSpan(operation_name);

  // Inject the span's context into the request headers.
  inject_span_context(tracer, request, span.context());

  return span;
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
  auto tracer = lightstep::Tracer::Global();
  auto parent_span_context = extract_span_context(tracer, request_);

  request_span_ = tracer.StartSpan(
      get_request_operation_name(request_, core_loc_conf, loc_conf_),
      {lightstep::SpanReference{lightstep::ChildOfRef, parent_span_context}});

  span_ = tracer.StartSpan(
      get_loc_operation_name(request_, core_loc_conf, loc_conf_),
      {lightstep::SpanReference{lightstep::ChildOfRef,
                                request_span_.context()}});

  inject_span_context(tracer, request_, span_.context());
}

//------------------------------------------------------------------------------
// on_enter_block
//------------------------------------------------------------------------------
void OpenTracingRequestInstrumentor::on_enter_block(
    ngx_http_core_loc_conf_t *core_loc_conf, opentracing_loc_conf_t *loc_conf) {
  on_exit_block();

  span_.Finish();

  auto tracer = lightstep::Tracer::Global();
  span_ = start_span(request_, core_loc_conf, loc_conf, tracer, span_.context(),
                     lightstep::FollowsFromRef);
  loc_conf_ = loc_conf;
}

//------------------------------------------------------------------------------
// on_exit_block
//------------------------------------------------------------------------------
void OpenTracingRequestInstrumentor::on_exit_block() {
  // Set default and custom tags for the block. Many nginx variables won't be
  // available when a block is first entered, so set tags when the block is
  // exited instead.
  add_script_tags(main_conf_->tags, request_, span_);
  add_script_tags(loc_conf_->tags, request_, span_);
}

//------------------------------------------------------------------------------
// on_log_request
//------------------------------------------------------------------------------
void OpenTracingRequestInstrumentor::on_log_request() {
  on_exit_block();

  // Check for errors.
  // TODO: Should we also look at request->err_status?
  auto status = uint64_t{request_->headers_out.status};
  const auto &status_line = request_->headers_out.status_line;
  span_.SetTag("http.status_code", status);
  if (status_line.data)
    span_.SetTag("http.status_line", to_string(status_line));
  // Treat any 4xx and 5xx code as an error.
  if (status >= 400) {
    span_.SetTag("error", true);
    // TODO: Log error values in request->headers_out.status_line to span.
  }

  add_script_tags(main_conf_->tags, request_, request_span_);

  span_.Finish();
  request_span_.Finish();
}
}  // namespace ngx_opentracing
