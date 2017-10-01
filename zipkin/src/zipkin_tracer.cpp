#include <ngx_opentracing_tracer_options.h>
#include "utility.h"
#include <opentracing/noop.h>
#include <zipkin/opentracing.h>
#include <string>
using namespace zipkin;
using namespace opentracing;

namespace ngx_opentracing {
std::shared_ptr<opentracing::Tracer> make_tracer(
    const tracer_options_t &options) {
  ZipkinOtTracerOptions tracer_options;
  if (options.collector_host.data)
    tracer_options.collector_host = to_string(options.collector_host);
  if (options.collector_port.data)
    // TODO: Check for errors here?
    tracer_options.collector_port =
        std::stoi(to_string(options.collector_port));
  if (options.service_name.data)
    tracer_options.service_name = to_string(options.service_name);
  else
    tracer_options.service_name = "nginx";
  return makeZipkinOtTracer(tracer_options);
}
}  // namespace ngx_opentracing
