Directives
----------

### `opentracing`

- **syntax** `opentracing on|off`
- **default**: `off`
- **context**: `http`, `server`, `location`

Enables or disables OpenTracing for NGINX requests.

### `opentracing_load_tracer`

- **syntax** `opentracing_load_tracer <tracer_library> <tracer_config_file>`
- **context**: `http`, `server`

Dynamicaly loads in a tracer implementing the OpenTracing dynamic loading interface.
`tracer_library` provides the path to the tracer's OpenTracing plugin, and `tracer_config_file`
provides a path to a JSON configuration for the tracer. See the vendor's documentation for
details on the format of the JSON configuration.

### `opentracing_propagate_context`

- **syntax** `opentracing_propagate_context`
- **context**: `http`, `server`, `location`

Propagates the active span context for upstream requests. (See
[cross-process-tracing](http://opentracing.io/documentation/pages/api/cross-process-tracing.html))

### `opentracing_fastcgi_propagate_context`

- **syntax** `opentracing_fastcgi_propagate_context`
- **context**: `http`, `server`, `location`

Propagates the active span context for FastCGI requests. (See
[cross-process-tracing](http://opentracing.io/documentation/pages/api/cross-process-tracing.html))

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

Variables
---------

### `opentracing_context_`*name*

Expands to the a value of the active span context; the last part of the variable
name is the span context's header converted to lower case with dashes replaced by
underscores.

### `opentracing_binary_context`

Expands to a binary string representation of the active span.
