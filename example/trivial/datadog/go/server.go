package main

import (
	"flag"
	"fmt"
	"net/http"
	"time"

	opentracing "github.com/opentracing/opentracing-go"
	// See https://docs.datadoghq.com/tracing/setup/go/
	"gopkg.in/DataDog/dd-trace-go.v1/ddtrace/opentracer"
	"gopkg.in/DataDog/dd-trace-go.v1/ddtrace/tracer"
)

const (
	serviceName = "hello-server"
)

var collectorHost = flag.String("collector_host", "dd-agent", "Host for Datadog agent")
var collectorPort = flag.String("collector_port", "8126", "Port for Datadog agent")

func handler(w http.ResponseWriter, r *http.Request) {
	wireContext, err := opentracing.GlobalTracer().Extract(
		opentracing.HTTPHeaders,
		opentracing.HTTPHeadersCarrier(r.Header))
	span := opentracing.StartSpan("GET /", opentracing.ChildOf(wireContext))
	defer span.Finish()
	tm := time.Now().Format(time.RFC1123)
	w.Write([]byte("The time is " + tm))
}

func main() {
	flag.Parse()
	agentAddr := fmt.Sprintf("%v:%v", *collectorHost, *collectorPort)
	t := opentracer.New(tracer.WithServiceName(serviceName), tracer.WithAgentAddr(agentAddr))
	defer tracer.Stop()

	opentracing.SetGlobalTracer(t)
	http.HandleFunc("/", handler)
	http.ListenAndServe(":9001", nil)
}
