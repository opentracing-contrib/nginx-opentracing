#!/bin/bash

set -e

[ -z "${SRC_DIR}" ] && export SRC_DIR="`pwd`"
[ -z "${BUILD_DIR}" ] && export BUILD_DIR=/build
[ -z "${MODULE_DIR}" ] && export MODULE_DIR=/modules

if [[ "$1" == "system.testing" ]]; then
  docker build -t nginx-opentracing-test/nginx -f Dockerfile-test .
  cd test
  docker build -t nginx-opentracing-test/backend -f Dockerfile-backend .
  cd environment/grpc
  docker build -t nginx-opentracing-test/grpc-backend .
  cd -
  PYTHONPATH=environment/grpc python3 nginx_opentracing_test.py
  exit 0
elif [[ "$1" == "build" ]]; then
  mkdir -p "${BUILD_DIR}"
  ./ci/build_nginx_opentracing_module.sh
  exit 0
elif [[ "$1" == "module.binaries" ]]; then
  mkdir -p "${BUILD_DIR}"
  mkdir -p "${MODULE_DIR}"
  ./ci/build_module_binaries.sh
  exit 0
elif [[ "$1" == "push_docker_image" ]]; then
  echo "$DOCKER_PASSWORD" | docker login -u "$DOCKER_USERNAME" --password-stdin
  VERSION_TAG="`git describe --abbrev=0 --tags`"
  VERSION="${VERSION_TAG/v/}" 
  # nginx
  docker build -t opentracing/nginx-opentracing .
  docker tag opentracing/nginx-opentracing opentracing/nginx-opentracing:${VERSION}
  docker push opentracing/nginx-opentracing:${VERSION}
  docker tag opentracing/nginx-opentracing opentracing/nginx-opentracing:latest
  docker push opentracing/nginx-opentracing:latest

  # openresty
  docker build -t opentracing/openresty -f Dockerfile-openresty .
  docker tag opentracing/openresty opentracing/openresty:${VERSION}
  docker push opentracing/openresty:${VERSION}
  docker tag opentracing/openresty opentracing/openresty:latest
  docker push opentracing/openresty:latest
  exit 0
elif [[ "$1" == "release" ]]; then
  mkdir -p "${BUILD_DIR}"
  mkdir -p "${MODULE_DIR}"
  ./ci/build_module_binaries.sh
  ./ci/release.sh
  exit 0
else
  echo "Invalid do_ci.sh target"
  exit 1
fi
