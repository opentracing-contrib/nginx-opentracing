#include <lightstep/impl.h>
#include <lightstep/recorder.h>
#include <opentracing_tracer_options.h>
#include <string>

namespace ngx_opentracing {
lightstep::Tracer make_tracer(const tracer_options_t &options) {
  lightstep::TracerOptions tracer_options;
  // If `options.access_token` isn't specified return a no-op tracer.
  if (!options.access_token.data)
    return lightstep::Tracer{nullptr};
  else
    tracer_options.access_token.assign(
        reinterpret_cast<char *>(options.access_token.data),
        options.access_token.len);
  if (options.collector_host.data)
    tracer_options.collector_host.assign(
        reinterpret_cast<char *>(options.collector_host.data),
        options.collector_host.len);
  else
    tracer_options.collector_host = "collector-grpc.lightstep.com";
  if (options.collector_port.data)
    // TODO: Check for errors here?
    tracer_options.collector_port = std::stoi(
        std::string{reinterpret_cast<char *>(options.collector_port.data),
                    options.collector_port.len});
  else
    tracer_options.collector_port = 443;
  if (options.collector_encryption.data)
    tracer_options.collector_encryption.assign(
        reinterpret_cast<char *>(options.collector_encryption.data),
        options.collector_encryption.len);
  else
    tracer_options.collector_encryption = "tls";
  if (options.component_name.data)
    tracer_options.tracer_attributes["lightstep.component_name"].assign(
        reinterpret_cast<char *>(options.component_name.data),
        options.component_name.len);
  else
    tracer_options.tracer_attributes["lightstep.component_name"] = "nginx";
  lightstep::BasicRecorderOptions recorder_options;
  return NewLightStepTracer(tracer_options, recorder_options);
}
/* static lightstep::Tracer initialize_tracer(const std::string &options) { */
/*   // TODO: Will need a way to start an arbitrary tracer here, but to get
 * started */
/*   // just hard-code to the LightStep tracer. Also, `options` may encode
 * multiple */
/*   // settings; for now assume it's the access token. */
/*   lightstep::TracerOptions tracer_options; */
/*   tracer_options.access_token = options; */
/*   tracer_options.collector_host = "collector-grpc.lightstep.com"; */
/*   tracer_options.collector_port = 443; */
/*   tracer_options.collector_encryption = "tls"; */
/*   tracer_options.tracer_attributes["lightstep.component_name"] = "nginx"; */
/*   lightstep::BasicRecorderOptions recorder_options; */
/*   return NewLightStepTracer(tracer_options, recorder_options); */
/* } */
} // namespace ngx_opentracing
