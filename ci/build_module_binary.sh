#!/bin/bash

set -e

pushd "${BUILD_DIR}"
NGINX_VERSION=$1
wget -O nginx-release-${NGINX_VERSION}.tar.gz https://github.com/nginx/nginx/archive/release-${NGINX_VERSION}.tar.gz
tar zxf nginx-release-$NGINX_VERSION.tar.gz
cd nginx-release-$NGINX_VERSION

# Set up an export map so that symbols from the opentracing module don't
# clash with symbols from other libraries.
cat <<EOF > export.map
{
  global:
    ngx_*;
  local: *;
};
EOF

./auto/configure \
      --with-compat \
      --add-dynamic-module="${SRC_DIR}"/opentracing
make modules

# Statically linking won't work correctly unless g++ is used instead of gcc, and
# there doesn't seem to be any way to have nginx build with g++
# (-with-cc=/usr/bin/g++ will fail when compiling the c files), so manually
# redo the linking.
/usr/bin/g++-7 -o ngx_http_opentracing_module.so \
  objs/addon/src/*.o \
  objs/ngx_http_opentracing_module_modules.o \
  -static-libstdc++ -static-libgcc \
  -lopentracing \
  -Wl,--version-script="${PWD}/export.map" \
  -shared

TARGET_NAME=linux-amd64-nginx-${NGINX_VERSION}-ngx_http_module.so.tgz
tar czf ${TARGET_NAME} ngx_http_opentracing_module.so
cp ${TARGET_NAME} "${MODULE_DIR}"/
popd
