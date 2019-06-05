#!/bin/bash

set -e

[ -z "${NGINX_VERSION}" ] && export NGINX_VERSION="1.17.0"

cd "${BUILD_DIR}"
wget -O nginx-release-${NGINX_VERSION}.tar.gz https://github.com/nginx/nginx/archive/release-${NGINX_VERSION}.tar.gz
tar zxf nginx-release-${NGINX_VERSION}.tar.gz
cd nginx-release-${NGINX_VERSION}
 auto/configure \
  --with-debug \
  --add-dynamic-module="${SRC_DIR}/opentracing"
make VERBOSE=1

