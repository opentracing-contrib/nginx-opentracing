// Copyright (c) 2017 Uber Technologies, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package tracing

import (
	"fmt"
	"time"
  "os"

	"github.com/opentracing/opentracing-go"
	"github.com/uber/jaeger-client-go/config"
	"github.com/uber/jaeger-client-go/rpcmetrics"
	"github.com/uber/jaeger-lib/metrics"
	"go.uber.org/zap"

	"github.com/rnburn/hotrod-docker/pkg/log"
)

// Init creates a new instance of Jaeger tracer.
func Init(serviceName string, metricsFactory metrics.Factory, logger log.Factory) opentracing.Tracer {
  localAgentHost := os.Getenv("JAEGER_AGENT_HOST")
  if localAgentHost == "" {
    localAgentHost = "localhost"
  }
  localAgentPort := os.Getenv("JAEGER_AGENT_PORT")
  if localAgentPort == "" {
    localAgentPort = "6831"
  }
	cfg := config.Configuration{
		Sampler: &config.SamplerConfig{
			Type:  "const",
			Param: 1,
		},
		Reporter: &config.ReporterConfig{
			LogSpans:            false,
			BufferFlushInterval: 1 * time.Second,
      LocalAgentHostPort: fmt.Sprintf("%s:%s", localAgentHost, localAgentPort),
		},
	}
	tracer, _, err := cfg.New(
		serviceName,
		config.Logger(jaegerLoggerAdapter{logger.Bg()}),
		config.Observer(rpcmetrics.NewObserver(metricsFactory, rpcmetrics.DefaultNameNormalizer)),
	)
	if err != nil {
		logger.Bg().Fatal("cannot initialize Jaeger Tracer", zap.Error(err))
	}
	return tracer
}

type jaegerLoggerAdapter struct {
	logger log.Logger
}

func (l jaegerLoggerAdapter) Error(msg string) {
	l.logger.Error(msg)
}

func (l jaegerLoggerAdapter) Infof(msg string, args ...interface{}) {
	l.logger.Info(fmt.Sprintf(msg, args...))
}
