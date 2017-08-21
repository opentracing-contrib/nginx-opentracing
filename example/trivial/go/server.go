package main

import (
	"fmt"
	opentracing "github.com/opentracing/opentracing-go"
	zipkin "github.com/openzipkin/zipkin-go-opentracing"
	"net/http"
	"os"
)

const (
	serviceName        = "hello-server"
	hostPort           = "0.0.0.0:0"
	debug              = false
	zipkinHTTPEndpoint = "http://localhost:9411/api/v1/spans"
	sameSpan           = false
	traceID128Bit      = true
)

func handler(w http.ResponseWriter, r *http.Request) {
	wireContext, _ := opentracing.GlobalTracer().Extract(
		opentracing.HTTPHeaders,
		opentracing.HTTPHeadersCarrier(r.Header))
	span := opentracing.StartSpan(
		"/",
		opentracing.ChildOf(wireContext))
	defer span.Finish()
	fmt.Fprintf(w, "Hello, World!")
}

func main() {
	collector, err := zipkin.NewHTTPCollector(zipkinHTTPEndpoint)
	if err != nil {
		fmt.Printf("unable to create Zipkin HTTP collector: %+v\n", err)
		os.Exit(-1)
	}

	recorder := zipkin.NewRecorder(collector, debug, hostPort, serviceName)

	tracer, err := zipkin.NewTracer(
		recorder,
		zipkin.ClientServerSameSpan(sameSpan),
		zipkin.TraceID128Bit(traceID128Bit))
	if err != nil {
		fmt.Printf("unable to create Zipkin tracer: %+v\n", err)
		os.Exit(-1)
	}

	opentracing.InitGlobalTracer(tracer)
	http.HandleFunc("/", handler)
	http.ListenAndServe(":9001", nil)
}
