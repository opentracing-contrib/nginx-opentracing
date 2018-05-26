#!/bin/bash

set -e

BUILD_IMAGE=nginx-opentracing-ci
docker image inspect "$BUILD_IMAGE" &> /dev/null || {
  docker build -t "$BUILD_IMAGE" ci
}

# Uses the approach recommending in the blog post
#   http://jpetazzo.github.io/2015/09/03/do-not-use-docker-in-docker-for-ci/
# for running docker within the CI docker image
# DOCKER_ARGS="-v /var/run/docker.sock:/var/run/docker.sock -v $PWD:/src/nginx-opentracing -w /src/nginx-opentracing"
DOCKER_ARGS="-v $PWD:/src/nginx-opentracing -w /src/nginx-opentracing"

if [ -n "$1" ]; then
  docker run $DOCKER_ARGS -it "$BUILD_IMAGE" /bin/bash -lc "$1"
else
  docker run $DOCKER_ARGS --privileged -it "$BUILD_IMAGE" /bin/bash -l
fi
