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
// OpenTracingContext
//------------------------------------------------------------------------------
OpenTracingContext::OpenTracingContext(ngx_http_request_t *request,
                                       ngx_http_core_loc_conf_t *core_loc_conf,
                                       opentracing_loc_conf_t *loc_conf)
    : request_{request},
      main_conf_{
          static_cast<opentracing_main_conf_t *>(ngx_http_get_module_main_conf(
              request_, ngx_http_opentracing_module))},
      core_loc_conf_{core_loc_conf},
      loc_conf_{loc_conf} {
  auto tracer = opentracing::Tracer::Global();
  if (!tracer) throw std::runtime_error{"no global tracer set"};

  std::unique_ptr<opentracing::SpanContext> parent_span_context = nullptr;
  if (loc_conf_->trust_incoming_span) {
    parent_span_context = extract_span_context(*tracer, request_);
  }

  ngx_log_debug1(NGX_LOG_DEBUG_HTTP, request_->connection->log, 0,
                 "starting opentracing request span for %p", request_);
  request_span_ = tracer->StartSpan(
      get_request_operation_name(request_, core_loc_conf_, loc_conf_),
      {opentracing::ChildOf(parent_span_context.get()),
       opentracing::StartTimestamp{
           to_system_timestamp(request->start_sec, request->start_msec)}});
  if (!request_span_) throw std::runtime_error{"tracer->StartSpan failed"};

  if (loc_conf_->enable_locations) {
    ngx_log_debug3(
        NGX_LOG_DEBUG_HTTP, request_->connection->log, 0,
        "starting opentracing location span for \"%V\"(%p) in request %p",
        &core_loc_conf->name, loc_conf_, request_);
    span_ = tracer->StartSpan(
        get_loc_operation_name(request_, core_loc_conf_, loc_conf_),
        {opentracing::ChildOf(&request_span_->context())});
    if (!span_) throw std::runtime_error{"tracer->StartSpan failed"};
  }
}

//------------------------------------------------------------------------------
// on_change_block
//------------------------------------------------------------------------------
void OpenTracingContext::on_change_block(
    ngx_http_core_loc_conf_t *core_loc_conf, opentracing_loc_conf_t *loc_conf) {
  on_exit_block();
  core_loc_conf_ = core_loc_conf;
  loc_conf_ = loc_conf;

  if (loc_conf->enable_locations) {
    ngx_log_debug3(
        NGX_LOG_DEBUG_HTTP, request_->connection->log, 0,
        "starting opentracing location span for \"%V\"(%p) in request %p",
        &core_loc_conf->name, loc_conf_, request_);
    span_ = request_span_->tracer().StartSpan(
        get_loc_operation_name(request_, core_loc_conf, loc_conf),
        {opentracing::ChildOf(&request_span_->context())});
    if (!span_) throw std::runtime_error{"tracer->StartSpan failed"};
  }
}

//------------------------------------------------------------------------------
// active_span
//------------------------------------------------------------------------------
const opentracing::Span &OpenTracingContext::active_span() const {
  if (loc_conf_->enable_locations) {
    return *span_;
  } else {
    return *request_span_;
  }
}

//------------------------------------------------------------------------------
// on_exit_block
//------------------------------------------------------------------------------
void OpenTracingContext::on_exit_block(
    std::chrono::steady_clock::time_point finish_timestamp) {
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

    // If the location operation name is dependent upon a variable, it may not
    // have been available when the span was first created, so set the operation
    // name again.
    //
    // See on_log_request below
    span_->SetOperationName(
        get_loc_operation_name(request_, core_loc_conf_, loc_conf_));

    span_->Finish({opentracing::FinishTimestamp{finish_timestamp}});
  } else {
    add_script_tags(loc_conf_->tags, request_, *request_span_);
  }
}

//------------------------------------------------------------------------------
// on_log_request
//------------------------------------------------------------------------------
void OpenTracingContext::on_log_request() {
  auto finish_timestamp = std::chrono::steady_clock::now();

  on_exit_block(finish_timestamp);

  ngx_log_debug1(NGX_LOG_DEBUG_HTTP, request_->connection->log, 0,
                 "finishing opentracing request span for %p", request_);
  add_status_tags(request_, *request_span_);
  add_script_tags(main_conf_->tags, request_, *request_span_);

  // When opentracing_operation_name points to a variable and it can be
  // initialized or modified at any phase of the request, so set the
  // span operation name at request exit phase, which will take the latest
  // value of the variable pointed to opentracing_operation_name directive
  auto core_loc_conf = static_cast<ngx_http_core_loc_conf_t *>(
      ngx_http_get_module_loc_conf(request_, ngx_http_core_module));
  request_span_->SetOperationName(
      get_request_operation_name(request_, core_loc_conf, loc_conf_));

  request_span_->Finish({opentracing::FinishTimestamp{finish_timestamp}});
}

//------------------------------------------------------------------------------
// lookup_span_context_value
//------------------------------------------------------------------------------
// Expands the active span context into a list of key-value pairs and returns
// the value for `key` if it exists.
//
// Note: there's caching so that if lookup_span_context_value is repeatedly
// called for the same active span context, it will only be expanded once.
//
// See propagate_opentracing_context
ngx_str_t OpenTracingContext::lookup_span_context_value(
    opentracing::string_view key) {
  return span_context_querier_.lookup_value(request_, active_span(), key);
}

//------------------------------------------------------------------------------
// get_binary_context
//------------------------------------------------------------------------------
ngx_str_t OpenTracingContext::get_binary_context() const {
  const auto &span = active_span();
  std::ostringstream oss;
  auto was_successful = span.tracer().Inject(span.context(), oss);
  if (!was_successful) {
    throw std::runtime_error{was_successful.error().message()};
  }
  return to_ngx_str(request_->pool, oss.str());
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
