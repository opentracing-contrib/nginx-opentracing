#!/bin/bash

pushd "${BUILD_DIR}"
git clone -b v$OPENTRACING_VERSION https://github.com/opentracing/opentracing-cpp.git
cd opentracing-cpp
mkdir .build && cd .build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-fPIC" \
      -DBUILD_SHARED_LIBS=OFF \
      -DBUILD_TESTING=OFF \
      -DBUILD_MOCKTRACER=OFF \
      ..
make && make install
popd
