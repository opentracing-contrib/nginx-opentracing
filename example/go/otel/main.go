package main

import (
	"context"
	"fmt"
	"io/ioutil"
	"log"
	"math/rand"
	"net/http"
	"net/http/httptrace"
	"os"
	"strings"
	"time"

	"github.com/pkg/errors"

	"go.opentelemetry.io/contrib/instrumentation/net/http/httptrace/otelhttptrace"
	"go.opentelemetry.io/contrib/instrumentation/net/http/otelhttp"

	"go.opentelemetry.io/otel"
	"go.opentelemetry.io/otel/baggage"
	"go.opentelemetry.io/otel/exporters/otlp"
	"go.opentelemetry.io/otel/label"
	"go.opentelemetry.io/otel/propagation"
	"go.opentelemetry.io/otel/sdk/resource"
	sdktrace "go.opentelemetry.io/otel/sdk/trace"
	"go.opentelemetry.io/otel/semconv"
	"go.opentelemetry.io/otel/trace"
)

func main() {
	err := run()
	if err != nil {
		log.Fatal(err)
	}
}

func run() error {
	collector_addr := os.Getenv("OT_COLLECTOR_ADDR")
	if collector_addr == "" {
		collector_addr = "localhost:55680"
	}

	shutdown, err := initProvider("checkout", collector_addr)
	if err != nil {
		return err
	}
	defer shutdown()
	return server()
}

func server() error {
	log.Printf("Listen on port 8080")

	otelHandler := otelhttp.NewHandler(http.HandlerFunc(handler), "all")
	http.Handle("/", otelHandler)

	return http.ListenAndServe(":8080", nil)
}

func handler(w http.ResponseWriter, r *http.Request) {
	endpoint := r.URL.Path[1:]

	if endpoint == "ping" {
		fmt.Fprint(w, "OK")
		return
	}

	ctx := r.Context()
	tracer := otel.Tracer("test-tracer")
	log.Printf("tracer: %+v", tracer)

	commonLabels := []label.KeyValue{
		label.String("span.kind", "server"),
		label.String("http.method", "GET"),
		label.String("http.route", "/"),
	}

	ctx, span := tracer.Start(
		r.Context(),
		endpoint,
		trace.WithAttributes(commonLabels...))
	defer span.End()

	n := rand.Intn(10)
	for i := 0; i < n; i++ {
		_, span := tracer.Start(ctx, fmt.Sprintf("Sample-%d", i))
		log.Printf("Doing really hard work (%d / %d)\n", i+1, n)

		span.AddEvent("event in fee")
		<-time.After(200 * time.Millisecond)
		span.AddEvent("finish")
		span.End()
	}

	client := http.Client{Transport: otelhttp.NewTransport(http.DefaultTransport)}
	ctx = baggage.ContextWithValues(ctx,
		label.String("query", "echo"),
	)

	err := func(ctx context.Context) error {
		ctx, span = tracer.Start(ctx, "search", trace.WithAttributes(semconv.PeerServiceKey.String("google_search")))
		defer span.End()

		ctx = httptrace.WithClientTrace(ctx, otelhttptrace.NewClientTrace(ctx))
		req, _ := http.NewRequestWithContext(ctx, "GET", "https://google.com/?q=echo", nil)

		log.Printf("Searching...")
		res, err := client.Do(req)
		if err != nil {
			panic(err)
		}
		_, err = ioutil.ReadAll(res.Body)
		_ = res.Body.Close()

		return err
	}(ctx)

	if err != nil {
		panic(err)
	}

	fmt.Fprint(w, "Success!\n")
	fmt.Fprintf(w, "endpoint: %s\n", endpoint)
	fmt.Fprint(w, "headers:\n")
	for key, vals := range r.Header {
		fmt.Fprintf(w, "  %s : %s\n", key, strings.Join(vals, ", "))
		log.Printf("%s : %s", key, strings.Join(vals, ", "))
	}
}

// Initializes an OTLP exporter, and configures the corresponding trace and
// metric providers.
func initProvider(service_name, collector_address string) (func(), error) {
	log.Println("Initialize tracing...")
	ctx := context.Background()

	// If the OpenTelemetry Collector is running on a local cluster (minikube or
	// microk8s), it should be accessible through the NodePort service at the
	// `localhost:30080` address. Otherwise, replace `localhost` with the
	// address of your cluster. If you run the app inside k8s, then you can
	// probably connect directly to the service through dns
	exp, err := otlp.NewExporter(ctx,
		otlp.WithInsecure(),
		otlp.WithAddress(collector_address),
		//otlp.WithGRPCDialOption(grpc.WithBlock()), // useful for testing
	)
	if err != nil {
		return nil, errors.Wrap(err, "failed to create exporter")
	}

	res, err := resource.New(ctx,
		resource.WithAttributes(
			// the service name used to display traces in backends
			semconv.ServiceNameKey.String(service_name),
		),
	)
	if err != nil {
		return nil, errors.Wrap(err, "failed to create resource")
	}

	bsp := sdktrace.NewBatchSpanProcessor(exp)
	tracerProvider := sdktrace.NewTracerProvider(
		sdktrace.WithConfig(sdktrace.Config{DefaultSampler: sdktrace.AlwaysSample()}),
		sdktrace.WithResource(res),
		sdktrace.WithSpanProcessor(bsp),
	)

	// set global propagator to tracecontext (the default is no-op).
	otel.SetTextMapPropagator(propagation.TraceContext{})
	otel.SetTracerProvider(tracerProvider)

	log.Println("Close to finish")
	return func() {
		tracerProvider.Shutdown(ctx)
		exp.Shutdown(ctx)
	}, nil
}
