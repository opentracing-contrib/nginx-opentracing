module github.com/opentracing-contrib/nginx-opentracing/example/go/otel

go 1.16

require (
	github.com/pkg/errors v0.9.1
	go.opentelemetry.io/contrib/instrumentation/net/http/otelhttp v0.17.0
	go.opentelemetry.io/otel v0.20.0
	go.opentelemetry.io/otel/exporters/otlp v0.20.1
	go.opentelemetry.io/otel/sdk v0.20.0
	go.opentelemetry.io/otel/trace v0.20.0
)
