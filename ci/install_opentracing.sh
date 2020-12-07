#!/bin/bash

set -e

[ -z "${OPENTRACING_VERSION}" ] && export OPENTRACING_VERSION="v1.5.1"
# [ -z "${OPENTRACING_VERSION}" ] && export OPENTRACING_VERSION="v1.6.0"

# Build OpenTracing
cd /
git clone -b ${OPENTRACING_VERSION} https://github.com/opentracing/opentracing-cpp.git
cd opentracing-cpp
mkdir .build && cd .build
cmake \
  -DBUILD_MOCKTRACER=OFF \
  -DBUILD_TESTING=OFF \
  ..
make && make install
