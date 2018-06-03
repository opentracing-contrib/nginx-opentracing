package main

import (
	"flag"
	"fmt"
	opentracing "github.com/opentracing/opentracing-go"
	zipkin "github.com/openzipkin/zipkin-go-opentracing"
	"net/http"
	"os"
)

const (
	serviceName   = "hello-server"
	hostPort      = "0.0.0.0:0"
	debug         = false
	sameSpan      = false
	traceID128Bit = true
)

var collectorHost = flag.String("collector_host", "localhost", "Host for Zipkin Collector")
var collectorPort = flag.String("collector_port", "9411", "Port for Zipkin Collector")

func handler(w http.ResponseWriter, r *http.Request) {
  for k, v := range r.Header {
    fmt.Fprintf(w, "Header field %q, Value %q\n", k, v)
  }
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
	flag.Parse()
  zipkinHTTPEndpoint := "http://" + *collectorHost + ":" + *collectorPort + "/api/v1/spans"
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
