#!/bin/bash

set -e

[ -z "${SRC_DIR}" ] && export SRC_DIR="`pwd`"

if [[ "$1" == "system.testing" ]]; then
  docker build -t nginx-opentracing-test/nginx -f Dockerfile-test .
  cd test
  docker build -t nginx-opentracing-test/backend -f Dockerfile-backend .
  python3 nginx_opentracing_test.py
  exit 0
elif [[ "$1" == "release" ]]; then
  echo "$DOCKER_PASSWORD" | docker login -u "$DOCKER_USERNAME" --password-stdin
  VERSION_TAG="`git describe --abbrev=0 --tags`"
  docker build -t opentracing/nginx-opentracing .
  docker tag opentracing/nginx-opentracing opentracing/nginx-opentracing:${VERSION_TAG}
  docker push opentracing/nginx-opentracing:${VERSION_TAG}
  docker tag opentracing/nginx-opentracing opentracing/nginx-opentracing:latest
  docker push opentracing/nginx-opentracing:latest
  exit 0
else
  echo "Invalid do_ci.sh target"
  exit 1
fi
