#include <lightstep/impl.h>
#include <lightstep/recorder.h>
#include <ngx_opentracing_utility.h>
#include <opentracing_tracer_options.h>
#include <string>

namespace ngx_opentracing {
lightstep::Tracer make_tracer(const tracer_options_t &options) {
  lightstep::TracerOptions tracer_options;
  // If `options.access_token` isn't specified return a no-op tracer.
  if (!options.access_token.data)
    return lightstep::Tracer{nullptr};
  else
    tracer_options.access_token = to_string(options.access_token);
  if (options.collector_host.data)
    tracer_options.collector_host = to_string(options.collector_host);
  else
    tracer_options.collector_host = "collector-grpc.lightstep.com";
  if (options.collector_port.data)
    // TODO: Check for errors here?
    tracer_options.collector_port =
        std::stoi(to_string(options.collector_port));
  else
    tracer_options.collector_port = 443;
  if (options.collector_encryption.data)
    tracer_options.collector_encryption =
        to_string(options.collector_encryption);
  else
    tracer_options.collector_encryption = "tls";
  if (options.component_name.data)
    tracer_options.tracer_attributes["lightstep.component_name"] =
        to_string(options.component_name);
  else
    tracer_options.tracer_attributes["lightstep.component_name"] = "nginx";
  lightstep::BasicRecorderOptions recorder_options;
  return NewLightStepTracer(tracer_options, recorder_options);
}
} // namespace ngx_opentracing
