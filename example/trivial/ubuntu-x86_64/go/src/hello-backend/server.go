package main

import (
	"flag"
	"fmt"
  "time"
  "log"
	opentracing "github.com/opentracing/opentracing-go"
  "github.com/uber/jaeger-client-go"
  "github.com/uber/jaeger-client-go/config"
	"net/http"
)

const (
	serviceName   = "hello-server"
	hostPort      = "0.0.0.0:0"
	debug         = false
	sameSpan      = false
	traceID128Bit = true
)

var collectorHost = flag.String("collector_host", "localhost", "Host for Zipkin Collector")
var collectorPort = flag.String("collector_port", "6831", "Port for Zipkin Collector")

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
    cfg := config.Configuration{
    Sampler: &config.SamplerConfig{
        Type:  "const",
        Param: 1,
    },
    Reporter: &config.ReporterConfig{
        LocalAgentHostPort: *collectorHost + ":" + *collectorPort,
        LogSpans:            true,
        BufferFlushInterval: 1 * time.Second,
    },
    }
    closer, err := cfg.InitGlobalTracer(
        "backend",
        config.Logger(jaeger.StdLogger),
    )

	if err != nil {
		log.Printf("Could not initialize jaeger tracer: %s", err.Error())
		return
	}

  defer closer.Close()

	http.HandleFunc("/", handler)
	http.ListenAndServe(":9001", nil)
}
