#!/bin/bash

NGINX_VERSION=1.13.12
set -e
cd /src
wget -O nginx-release-${NGINX_VERSION}.tar.gz https://github.com/nginx/nginx/archive/release-${NGINX_VERSION}.tar.gz
tar zxf nginx-release-${NGINX_VERSION}.tar.gz
cd /src/nginx-release-${NGINX_VERSION}
export ASAN_OPTIONS=detect_leaks=0 # Need to temporarily turn off leak sanitizer otherwise we get an error when building
auto/configure \
  --with-compat \
  --with-debug \
  --with-cc-opt="-fno-omit-frame-pointer -fsanitize=address" \
  --with-ld-opt="-lasan -fno-omit-frame-pointer -fsanitize=address" \
  --add-dynamic-module=/src/nginx-opentracing/opentracing 
export ASAN_OPTIONS=detect_leaks=1
make && make install
export PATH=/usr/local/nginx/sbin:$PATH
NGINX_MODULES_PATH=/usr/local/nginx/modules/
cd /src/nginx-opentracing
export MOCKTRACER_LIBRARY=/usr/local/lib/libopentracing_mocktracer.so
export NGINX_OPENTRACING_MODULE="$NGINX_MODULES_PATH/ngx_http_opentracing_module.so"
export NGINX_OPENTRACING_TEST_DIR=/nginx-opentracing-test
export LD_LIBRARY_PATH=/usr/local/lib
if [ -d $NGINX_OPENTRACING_TEST_DIR ]; then
  rm -rf $NGINX_OPENTRACING_TEST_DIR
fi
mkdir $NGINX_OPENTRACING_TEST_DIR
chmod a+rwx $NGINX_OPENTRACING_TEST_DIR
cd test
for i in *;
do
  "$i/run.sh"
done
