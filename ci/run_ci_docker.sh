#!/bin/bash

set -e

BUILD_IMAGE=nginx-opentracing-ci
docker image inspect "$BUILD_IMAGE" &> /dev/null || {
  docker build -t "$BUILD_IMAGE" ci
}

if [ -n "$1" ]; then
  docker run -v "$PWD":/src/nginx-opentracing -w /src/nginx-opentracing -it "$BUILD_IMAGE" /bin/bash -lc "$1"
else
  docker run -v "$PWD":/src/nginx-opentracing -w /src/nginx-opentracing --privileged -it "$BUILD_IMAGE" /bin/bash -l
fi
