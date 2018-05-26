#!/bin/bash

set -e

[ -z "${SRC_DIR}" ] && export SRC_DIR="`pwd`"

if [[ "$1" == "system.testing" ]]; then
  docker build -t nginx-opentracing-test/nginx -f Dockerfile-test .
  cd test2
  docker build -t nginx-opentracing-test/backend -f Dockerfile-backend .
  python nginx_opentracing_test.py
  exit 0
fi
