#!/bin/bash

set -e

[ -z "${OPENTRACING_VERSION}" ] && export OPENTRACING_VERSION="v1.5.0"

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
