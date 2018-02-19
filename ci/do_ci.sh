#!/bin/bash

set -e

NGINX_VERSION=`nginx -v 2>&1` && NGINX_VERSION=${NGINX_VERSION#*nginx/}
NGINX_MODULES_PATH=$(nginx -V 2>&1 | grep -oP "modules-path=\K[^\s]*")
cd /src/nginx-release-${NGINX_VERSION}
auto/configure \
  --with-compat \
  --add-dynamic-module=/src/nginx-opentracing/opentracing 
make modules
cp objs/ngx_http_opentracing_module.so $NGINX_MODULES_PATH/
cd /src/nginx-opentracing
export MOCKTRACER_LIBRARY=/usr/local/lib/libopentracing_mocktracer.so
export NGINX_OPENTRACING_MODULE="$NGINX_MODULES_PATH/ngx_http_opentracing_module.so"
export NGINX_OPENTRACING_TEST_DIR=/nginx-opentracing-test
mkdir $NGINX_OPENTRACING_TEST_DIR
chmod a+rwx $NGINX_OPENTRACING_TEST_DIR
cd test
for i in *;
do
  "$i/run.sh"
done
