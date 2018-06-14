#!/bin/bash

set -e

[ -z "${SRC_DIR}" ] && export SRC_DIR="`pwd`"
[ -z "${BUILD_DIR}" ] && export BUILD_DIR=/build
[ -z "${MODULE_DIR}" ] && export MODULE_DIR=/modules


if [[ "$1" == "system.testing" ]]; then
  docker build -t nginx-opentracing-test/nginx -f Dockerfile-test .
  cd test
  docker build -t nginx-opentracing-test/backend -f Dockerfile-backend .
  cd environment
  docker-compose build
  cd ..
  python3 nginx_opentracing_test.py
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
  docker build -t opentracing/nginx-opentracing .
  docker tag opentracing/nginx-opentracing opentracing/nginx-opentracing:${VERSION}
  docker push opentracing/nginx-opentracing:${VERSION}
  docker tag opentracing/nginx-opentracing opentracing/nginx-opentracing:latest
  docker push opentracing/nginx-opentracing:latest
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
