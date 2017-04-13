#include "opentracing_request_processor.h"
#include "ngx_http_opentracing_conf.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <lightstep/impl.h>
#include <lightstep/recorder.h>
#include <ngx_opentracing_utility.h>

extern "C" {
extern ngx_module_t ngx_http_opentracing_module;
}

namespace ngx_opentracing {
lightstep::SpanContext extract_span_context(lightstep::Tracer &tracer,
                                            const ngx_http_request_t *request);

void inject_span_context(lightstep::Tracer &tracer, ngx_http_request_t *request,
                         const lightstep::SpanContext span_context);

lightstep::Tracer make_tracer(const tracer_options_t &);

static bool
is_opentracing_enabled(const ngx_http_request_t *request,
                       const ngx_http_core_loc_conf_t *core_loc_conf,
                       const opentracing_loc_conf_t *loc_conf) {
  if (request == request->main)
    return loc_conf->enable;
  else
    // Only trace subrequests if `log_subrequest` is enabled; otherwise the
    // spans won't be finished.
    return loc_conf->enable && core_loc_conf->log_subrequest;
}

static void add_script_tag(ngx_http_request_t *request, lightstep::Span &span,
                           const opentracing_tag_t &tag) {
  auto key = tag.key_script.run(request);
  auto value = tag.value_script.run(request);
  if (key.data && value.data)
    span.SetTag(to_string(key), to_string(value));
}

static lightstep::Span
start_span(ngx_http_request_t *request,
           const ngx_http_core_loc_conf_t *core_loc_conf,
           const opentracing_loc_conf_t *loc_conf, lightstep::Tracer &tracer,
           const lightstep::SpanContext &reference_span_context,
           lightstep::SpanReferenceType reference_type) {
  auto main_conf = static_cast<opentracing_main_conf_t *>(
      ngx_http_get_module_main_conf(request, ngx_http_opentracing_module));

  // Start a new span for the location block.
  std::string operation_name;
  if (loc_conf->operation_name_script.is_valid())
    operation_name = to_string(loc_conf->operation_name_script.run(request));
  else
    operation_name = to_string(core_loc_conf->name);
  lightstep::Span span;
  if (reference_span_context.valid()) {
    std::cerr << "starting child span " << operation_name << "...\n";
    span = tracer.StartSpan(
        operation_name,
        {lightstep::SpanReference{reference_type, reference_span_context}});
  } else {
    std::cerr << "starting span " << operation_name << "...\n";
    span = tracer.StartSpan(operation_name);
  }

  // Set default span tags.
  if (main_conf->tags)
    for_each<opentracing_tag_t>(*main_conf->tags,
                                [&](const opentracing_tag_t &tag) {
                                  add_script_tag(request, span, tag);
                                });

  // Set custom span tags.
  if (loc_conf->tags)
    for_each<opentracing_tag_t>(*loc_conf->tags,
                                [&](const opentracing_tag_t &tag) {
                                  add_script_tag(request, span, tag);
                                });

  // Inject the span's context into the request headers.
  inject_span_context(tracer, request, span.context());

  return span;
}

OpenTracingRequestProcessor::OpenTracingRequestProcessor(
    const tracer_options_t &options)
    : tracer_{make_tracer(options)} {}

void OpenTracingRequestProcessor::before_response(ngx_http_request_t *request) {
  auto core_loc_conf = static_cast<ngx_http_core_loc_conf_t *>(
      ngx_http_get_module_loc_conf(request, ngx_http_core_module));
  auto loc_conf = static_cast<opentracing_loc_conf_t *>(
      ngx_http_get_module_loc_conf(request, ngx_http_opentracing_module));

  auto span_iter = active_spans_.find(request);
  if (span_iter == active_spans_.end()) {
    if (!is_opentracing_enabled(request, core_loc_conf, loc_conf))
      return;
    // No span has been created for this request yet. Check if there's any
    // parent span context in the request headers and start a new one.
    auto parent_span_context = extract_span_context(tracer_, request);
    auto span = start_span(request, core_loc_conf, loc_conf, tracer_,
                           parent_span_context, lightstep::ChildOfRef);
    active_spans_.emplace(request, std::move(span));
  } else {
    // A span's already been created for the request, but nginx is entering a
    // new location block. Finish the span for the previous location block and
    // create a new span that follows from it.
    auto &span = span_iter->second;
    span.Finish();
    span = start_span(request, core_loc_conf, loc_conf, tracer_, span.context(),
                      lightstep::FollowsFromRef);
  }
}

void OpenTracingRequestProcessor::after_response(ngx_http_request_t *request) {
  // Lookup the span.
  auto span_iter = active_spans_.find(request);
  if (span_iter == std::end(active_spans_))
    return;

  auto &span = span_iter->second;

  // Check for errors.
  // TODO: Should we also look at request->err_status?
  auto status = uint64_t{request->headers_out.status};
  const auto &status_line = request->headers_out.status_line;
  span.SetTag("http.status_code", status);
  if (status_line.data)
    span.SetTag("http.status_line", to_string(status_line));

  // Treat any 4xx and 5xx code as an error.
  if (status >= 400) {
    span.SetTag("error", true);
    // TODO: Log error values in request->headers_out.status_line to span.
  }

  // Finish the span.
  span.Finish();
  active_spans_.erase(span_iter);
}
} // namespace ngx_opentracing
