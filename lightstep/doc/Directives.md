### `lightstep_access_token`

- **syntax** `lightstep_access_token <access_token>`
- **context**: `http`

Specifies the access token to use when uploading traces.

### `lightstep_component_name`

- **syntax** `lightstep_component_name <component_name>`
- **default**: `nginx`
- **context**: `http`

Specifies the component name to use for any traces created.

### `lightstep_collector_host`

- **syntax** `lightstep_collector_host <host>`
- **default**: `collector-grpc.lightstep.com`
- **context**: `http`

Specifies the host to use when uploading traces.

### `lightstep_collector_plaintext`

- **syntax** `lightstep_collector_plaintext on|off`
- **default**: `off`
- **context**: `http`

Specifies whether to encrypt when uploading traces.

### `lightstep_collector_port`

- **syntax** `lightstep_collector_port <port>`
- **default**: `443`
- **context**: `http`

Specifies the port to use when uploading traces.
