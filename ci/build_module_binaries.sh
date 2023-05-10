#!/bin/bash

set -e

[ -z "${OPENTRACING_VERSION}" ] && export OPENTRACING_VERSION="v1.6.0"
NAME_SUFFIX=${NAME_SUFFIX:-""}

NGINX_VERSIONS=(1.24.0 1.23.4 1.23.3 1.23.2 1.23.1 1.23.0 1.22.1 1.22.0 1.21.6 1.21.5 1.21.4 1.21.3 1.21.2 1.21.1 1.21.0 1.20.2 1.20.1 1.20.0 1.19.10 1.19.9 1.19.8 1.19.7 1.19.6 1.19.5 1.19.4 1.19.3 1.19.2 1.18.0 1.17.8 1.17.3 1.17.2 1.17.1 1.17.0 1.16.1 1.16.0 1.15.8 1.15.1 1.15.0 1.14.2 1.13.6)

# Compile for a portable cpu architecture
export CFLAGS="-march=x86-64 -fPIC"
export CXXFLAGS="-march=x86-64 -fPIC"
export LDFLAGS="-fPIC"

./ci/build_static_opentracing.sh

for NGINX_VERSION in ${NGINX_VERSIONS[*]}; do
  ./ci/build_module_binary.sh "${NGINX_VERSION}" "${NAME_SUFFIX}"
done
