### `opentracing`

- **syntax** `opentracing on|off`
- **default**: `off`
- **context**: `http`, `server`, `location`

Enables or disables OpenTracing for NGINX requests.

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

### `opentracing_tag`

- **syntax** `opentracing_tag <key> <value>`
- **context**: `http`, `server`, `location`

Sets a [tag](https://github.com/opentracing/specification/blob/master/specification.md#set-a-span-tag)
with the given `key` and `value` for an NGINX span.
