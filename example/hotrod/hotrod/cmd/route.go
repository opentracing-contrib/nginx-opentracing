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

package cmd

import (
	"net"
	"strconv"

	"github.com/spf13/cobra"
	"go.uber.org/zap"

	"github.com/rnburn/hotrod-docker/pkg/log"
	"github.com/rnburn/hotrod-docker/pkg/tracing"
	"github.com/rnburn/hotrod-docker/services/route"
)

// routeCmd represents the route command
var routeCmd = &cobra.Command{
	Use:   "route",
	Short: "Starts Route service",
	Long:  `Starts Route service.`,
	RunE: func(cmd *cobra.Command, args []string) error {
		logger := log.NewFactory(logger.With(zap.String("service", "route")))
		server := route.NewServer(
			net.JoinHostPort(routeOptions.serverInterface, strconv.Itoa(routeOptions.serverPort)),
			tracing.Init("route", metricsFactory.Namespace("route", nil), logger),
			logger,
		)
		return server.Run()
	},
}

var (
	routeOptions struct {
		serverInterface string
		serverPort      int
	}
)

func init() {
	RootCmd.AddCommand(routeCmd)

	routeCmd.Flags().StringVarP(&routeOptions.serverInterface, "bind", "", "127.0.0.1", "interface to which the Route server will bind")
	routeCmd.Flags().IntVarP(&routeOptions.serverPort, "port", "p", 8083, "port on which the Route server will listen")
}
