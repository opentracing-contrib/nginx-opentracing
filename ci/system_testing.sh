#!/bin/bash

./ci/install_dependencies.sh

set -x
mkdir ./test-log
chmod a+rwx ./test-log
export LOG_DIR=$PWD/test-log
./ci/do_ci.sh system.testing
