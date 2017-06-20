#include <opentracing/noop.h>
#include <lightstep/tracer.h> 
#include <ngx_opentracing_tracer_options.h>
#include <ngx_opentracing_utility.h>
#include <string>
using namespace lightstep;
using namespace opentracing;

namespace ngx_opentracing {
std::shared_ptr<Tracer> make_tracer(const tracer_options_t &options) {
  LightStepTracerOptions tracer_options;
  // If `options.access_token` isn't specified return a no-op tracer.
  if (!options.access_token.data)
    return make_noop_tracer();
  else
    tracer_options.access_token = to_string(options.access_token);
  if (options.collector_host.data)
    tracer_options.collector_host = to_string(options.collector_host);
  if (options.collector_port.data)
    // TODO: Check for errors here?
    tracer_options.collector_port =
        std::stoi(to_string(options.collector_port));
  if (options.collector_encryption.data)
    tracer_options.collector_encryption =
        to_string(options.collector_encryption);
  if (options.component_name.data)
    tracer_options.component_name = to_string(options.component_name);
  else
    tracer_options.component_name = "nginx";
  return lightstep::make_lightstep_tracer(tracer_options);
}
}  // namespace ngx_opentracing
