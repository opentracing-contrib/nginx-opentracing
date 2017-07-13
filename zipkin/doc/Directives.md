### `zipkin_service_name`

- **syntax** `zipkin_service_name <service_name>`
- **default**: `nginx`
- **context**: `http`

Specifies the service name to use for any traces created.

### `zipkin_collector_host`

- **syntax** `zipkin_collector_host <host>`
- **default**: `NA`
- **context**: `http`

Specifies the host to use when uploading traces.

### `zipkin_collector_port`

- **syntax** `zipkin_collector_port <port>`
- **default**: `9411`
- **context**: `http`

Specifies the port to use when uploading traces.
