### `jaeger_service_name`

- **syntax** `jaeger_service_name <service_name>`
- **context**: `http`

Specifies the service name to use for traces.

### `jaeger_disabled`

- **syntax** `jaeger_disabled on|off`
- **default**: `off`
- **context**: `http`

Specifies whether or not to disable tracing.

### `jaeger_sampler_type`

- **syntax** `jaeger_sampler_type <sampler_type>`
- **default**: `remote`
- **context**: `http`

Specifies the sampler to be used when sampling traces. The available samplers
are: `const`, `probabilistic`, `ratelimiting`, `remote`. `const` sampling always
sample or always ignore. `probabilistic` sampling will use a random number to
check whether or not to sample the trace. `ratelimiting` sampling caps the
number of traces at a given number per second. `remote` sampling allows the
Jaeger backend to customize the sampling strategy based on throughput. The
recommended default is `remote`.

### `jaeger_sampler_param`

- **syntax** `jaeger_sampler_param <sampler_param>`
- **default**: `0.001`
- **context**: `http`

Specifies the argument to be passed to the sampler constructor. Must be a
number. For `const` this should be `0` to never sample and `1` to always sample.
For `probabilistic`, this should be a number representing the percent to sample
(i.e. 0.001 = 0.1%). For `ratelimiting` this should be the maximum number of
traces to sample per second. For `remote` this represents the upfront
probabilistic sampling rate to use for traces before determining throughput on
the backend (i.e. 0.001 = 0.1%, as above).

### `jaeger_sampling_server_url`

- **syntax** `jaeger_sampling_server_url <sampling_server_url>`
- **default**: `http://127.0.0.1:5778`
- **context**: `http`

Specifies the URL to use to request sampling strategy from Jaeger agent.

### `jaeger_sampler_max_operations`

- **syntax** `jaeger_sampler_max_operations <sampler_max_operations>`
- **default**: `2000`
- **context**: `http`

Specifies the maximum number of operations that the sampler will keep track of.

### `jaeger_sampling_refresh_interval_seconds`

- **syntax** `jaeger_sampling_refresh_interval_seconds <sampling_refresh_interval_seconds>`
- **default**: `60`
- **context**: `http`

Specifies the interval the remote sampler should wait before polling the Jaeger
agent again for sampling strategy information.

### `jaeger_reporter_queue_size`

- **syntax** `jaeger_reporter_queue_size <reporter_queue_size>`
- **default**: `100`
- **context**: `http`

Specifies the maximum number of spans to maintain in memory awaiting submission
to Jaeger agent before dropping new spans.

### `jaeger_reporter_buffer_flush_interval_seconds`

- **syntax** `jaeger_reporter_buffer_flush_interval_seconds <reporter_buffer_flush_interval_seconds>`
- **default**: `10`
- **context**: `http`

Specifies the maximum number of seconds to wait before forcing a flush of spans
to Jaeger agent, even if size does not warrant flush.

### `jaeger_reporter_log_spans`

- **syntax** `jaeger_reporter_log_spans <reporter_log_spans>`
- **default**: `false`
- **context**: `http`

Specifies whether or not the reporter should log the spans it reports.

### `jaeger_reporter_local_agent_host_port`

- **syntax** `jaeger_reporter_local_agent_host_port <reporter_local_agent_host_port>`
- **default**: `127.0.0.1:6831`
- **context**: `http`

Specifies the host and port of Jaeger agent where reporter should submit span
data.

### `jaeger_debug_header`

- **syntax** `jaeger_debug_header <debug_header>`
- **default**: `jaeger-debug-id`
- **context**: `http`

Specifies the name of HTTP header or a TextMap carrier key which, if found in
the carrier, forces the trace to be sampled as "debug" trace.

### `jaeger_baggage_header`

- **syntax** `jaeger_baggage_header <baggage_header>`
- **default**: `jaeger-baggage`
- **context**: `http`

Specifies the name of the HTTP header that is used to submit baggage.
It differs from `jaeger_trace_baggage_header_prefix` in that it can be used only
in cases where a root span does not exist.

### `jaeger_trace_context_header_name`

- **syntax** `jaeger_trace_context_header_name <trace_context_header_name>`
- **default**: `uber-trace-id`
- **context**: `http`

Specifies the HTTP header name used to propagate tracing context. This must be
lowercase to avoid mismatches when decoding incoming headers.

### `jaeger_trace_baggage_header_prefix`

- **syntax** `jaeger_trace_baggage_header_prefix <trace_baggage_header_prefix>`
- **default**: `uberctx-`
- **context**: `http`

Specifies the prefix for HTTP headers used to propagate baggage. This must be
lowercase to avoid mismatches when decoding incoming headers.
