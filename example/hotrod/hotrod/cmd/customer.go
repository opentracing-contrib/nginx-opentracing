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
	"github.com/rnburn/hotrod-docker/services/customer"
)

// customerCmd represents the customer command
var customerCmd = &cobra.Command{
	Use:   "customer",
	Short: "Starts Customer service",
	Long:  `Starts Customer service.`,
	RunE: func(cmd *cobra.Command, args []string) error {
		logger := log.NewFactory(logger.With(zap.String("service", "customer")))
		server := customer.NewServer(
			net.JoinHostPort(customerOptions.serverInterface, strconv.Itoa(customerOptions.serverPort)),
			tracing.Init("customer", metricsFactory.Namespace("customer", nil), logger),
			metricsFactory,
			logger,
		)
		return server.Run()
	},
}

var (
	customerOptions struct {
		serverInterface string
		serverPort      int
	}
)

func init() {
	RootCmd.AddCommand(customerCmd)

	customerCmd.Flags().StringVarP(&customerOptions.serverInterface, "bind", "", "127.0.0.1", "interface to which the Customer server will bind")
	customerCmd.Flags().IntVarP(&customerOptions.serverPort, "port", "p", 8081, "port on which the Customer server will listen")
}
