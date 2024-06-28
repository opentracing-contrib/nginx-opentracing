#!/bin/bash

set -e

[ -z "${SRC_DIR}" ] && export SRC_DIR="$(pwd)"
[ -z "${BUILD_DIR}" ] && export BUILD_DIR=/build

if [[ "$1" == "build" ]]; then
  mkdir -p "${BUILD_DIR}"
  ./ci/build_nginx_opentracing_module.sh
  exit 0
else
  echo "Invalid do_ci.sh target"
  exit 1
fi
