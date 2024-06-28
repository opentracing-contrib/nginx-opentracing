FROM debian:12 AS base

RUN apt-get update && \
    apt-get install --no-install-recommends --no-install-suggests -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    ca-certificates \
    automake \
    autogen \
    autoconf \
    libtool \
    ssh \
    wget \
    libpcre3 libpcre3-dev \
    zlib1g-dev

ENV CFLAGS="-fPIC"
ENV CXXFLAGS="-fPIC"
ENV LDFLAGS="-fPIC"


FROM base AS opentracing
ARG OPENTRACING_VERSION=v1.6.0

RUN <<"eot" bash -euo pipefail
    git clone -b "${OPENTRACING_VERSION}" https://github.com/opentracing/opentracing-cpp.git
    cd opentracing-cpp
    mkdir .build && cd .build
    cmake -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-fPIC" \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_TESTING=OFF \
        -DBUILD_MOCKTRACER=OFF \
        ..
    make && make install
eot


FROM opentracing AS binary
ARG NGINX_VERSION

COPY --link opentracing /opentracing

ADD --link https://github.com/nginx/nginx/archive/release-${NGINX_VERSION}.tar.gz /
RUN tar zxf release-${NGINX_VERSION}.tar.gz
WORKDIR /nginx-release-${NGINX_VERSION}

COPY <<-EOT export.map
{
  global:
    ngx_*;
  local: *;
};
EOT

RUN ./auto/configure \
    --with-compat \
    --add-dynamic-module=/opentracing \
    && make modules

RUN /usr/bin/g++ -o ngx_http_opentracing_module.so \
    objs/addon/src/*.o \
    objs/ngx_http_opentracing_module_modules.o \
    -static-libstdc++ -static-libgcc \
    -lopentracing \
    -Wl,--version-script="export.map" \
    -shared


FROM scratch AS export
ARG NGINX_VERSION
COPY --from=binary /nginx-release-${NGINX_VERSION}/ngx_http_opentracing_module.so /
