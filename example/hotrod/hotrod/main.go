// Copyright (c) 2017 Uber Technologies, Inc.

package main

import (
	"runtime"

	"github.com/rnburn/hotrod-docker/cmd"
)

func main() {
	runtime.GOMAXPROCS(runtime.NumCPU())
	cmd.Execute()
}
