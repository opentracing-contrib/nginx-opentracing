### `opentracing`

- **syntax** `opentracing on|off`
- **default**: `off`
- **context**: `http`, `server`, `location`

Enables or disables OpenTracing for NGINX requests.

### `opentracing_propagate_context`

- **syntax** `opentracing_propagate_context`
- **context**: `location`

Propagates the active span context for upstream requests. See
[cross-process-tracing](http://opentracing.io/documentation/pages/api/cross-process-tracing.html).

### `opentracing_load_tracer`

- **syntax** `opentracing_load_tracer <tracer_library> <tracer_config_file>`
- **context**: `http`, `server`

Dynamicaly loads in a tracer implementing the OpenTracing dynamic loading interface.

### `opentracing_trace_locations`

- **syntax** `opentracing_trace_locations on|off`
- **default**: `on`
- **context**: `http`, `server`, `location`

Enables the creation of OpenTracing spans for location blocks within a request.

### `opentracing_operation_name`

- **syntax** `opentracing_operation_name <operation_name>`
- **default**: The name of the first location block entered.
- **context**: `http`, `server`, `location`

Sets the [operation name](https://github.com/opentracing/specification/blob/master/specification.md#start-a-new-span)
of the span for an NGINX request.

### `opentracing_location_operation_name`

- **syntax** `opentracing_location_operation_name <operation_name>`
- **default**: The name of the location block.
- **context**: `http`, `server`, `location`

Sets the [operation name](https://github.com/opentracing/specification/blob/master/specification.md#start-a-new-span)
of the span for an NGINX location block.

### `opentracing_trust_incoming_span`

- **syntax** `opentracing_trust_incoming_span on|off`
- **default**: `on`
- **context**: `http`, `server`, `location`

Enables or disables using OpenTracing spans from incoming requests as parent for created ones. Might be disabled for security reasons.

### `opentracing_tag`

- **syntax** `opentracing_tag <key> <value>`
- **context**: `http`, `server`, `location`

Sets a [tag](https://github.com/opentracing/specification/blob/master/specification.md#set-a-span-tag)
with the given `key` and `value` for an NGINX span.
