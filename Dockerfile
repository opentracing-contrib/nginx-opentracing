# syntax=docker/dockerfile:1.3
ARG BUILD_OS=debian
FROM --platform=$BUILDPLATFORM tonistiigi/xx:1.1.1 AS xx

### Build base image for debian
FROM --platform=$BUILDPLATFORM debian:bullseye as build-base-debian

RUN apt-get update \
    && apt-get install --no-install-recommends --no-install-suggests -y \
    build-essential \
    ca-certificates \
    clang \
    git \
    golang \
    libcurl4 \
    libprotobuf-dev \
    libtool \
    libyaml-cpp-dev \
    libz-dev \
    lld \
    pkg-config \
    protobuf-compiler \
    wget

COPY --from=xx / /
ARG TARGETPLATFORM

RUN xx-apt install -y xx-cxx-essentials zlib1g-dev libcurl4-openssl-dev libc-ares-dev libre2-dev libssl-dev libc-dev libmsgpack-dev


### Build base image for alpine
FROM --platform=$BUILDPLATFORM alpine:3.16 as build-base-alpine

RUN apk add --no-cache \
    alpine-sdk \
    bash \
    build-base \
    clang \
    gcompat \
    git \
    libcurl \
    lld \
    protobuf-dev \
    yaml-cpp-dev \
    zlib-dev

COPY --from=xx / /
ARG TARGETPLATFORM

RUN xx-apk add --no-cache xx-cxx-essentials openssl-dev zlib-dev zlib libgcc curl-dev msgpack-cxx-dev


### Build image
FROM build-base-${BUILD_OS} as build-base

ENV CMAKE_VERSION 3.22.2
RUN wget -q -O cmake-linux.sh "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-$(arch).sh" \
    && sh cmake-linux.sh -- --skip-license --prefix=/usr \
    && rm cmake-linux.sh

# XX_CC_PREFER_STATIC_LINKER prefers ld to lld in ppc64le and 386.
ENV XX_CC_PREFER_STATIC_LINKER=1


## Build gRPC
FROM build-base as grpc
ARG GRPC_VERSION=v1.40.x
ARG TARGETPLATFORM

RUN xx-info env && git clone --depth 1 -b $GRPC_VERSION https://github.com/grpc/grpc \
    && cd grpc\
    # Get absl
    && git submodule update --depth 1 --init -- "third_party/abseil-cpp" \
    # Get protobuf
    && git submodule update --depth 1 --init -- "third_party/protobuf" \
    && mkdir .build && cd .build \
    && cmake $(xx-clang --print-cmake-defines) \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DgRPC_INSTALL=ON \
    -DgRPC_BUILD_TESTS=OFF \
    -DgRPC_ABSL_PROVIDER=module     \
    -DgRPC_CARES_PROVIDER=package    \
    -DgRPC_PROTOBUF_PROVIDER=module \
    -DgRPC_RE2_PROVIDER=package      \
    -DgRPC_SSL_PROVIDER=package      \
    -DgRPC_ZLIB_PROVIDER=package \
    .. \
    && make -j$(nproc) install


### Build opentracing-cpp
FROM build-base as opentracing-cpp
ARG OPENTRACING_CPP_VERSION=v1.6.0
ARG TARGETPLATFORM

RUN xx-info env && git clone --depth 1 -b $OPENTRACING_CPP_VERSION https://github.com/opentracing/opentracing-cpp.git \
    && cd opentracing-cpp \
    && mkdir .build && cd .build \
    && cmake $(xx-clang --print-cmake-defines) \
    -DCMAKE_INSTALL_PREFIX=$(xx-info sysroot)usr/local \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_STATIC_LIBS=ON \
    -DBUILD_MOCKTRACER=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_TESTING=OFF .. \
    && make -j$(nproc) install \
    && if [ "$(xx-info sysroot)" != "/" ]; then cp -a $(xx-info sysroot)usr/local/lib/libopentracing.so* /usr/local/lib/; fi \
    && xx-verify /usr/local/lib/libopentracing.so


### Build zipkin-cpp-opentracing
FROM opentracing-cpp as zipkin-cpp-opentracing
ARG ZIPKIN_CPP_VERSION=master
ARG TARGETPLATFORM

RUN [ "$(xx-info vendor)" = "alpine" ] && export QEMU_LD_PREFIX=/$(xx-info); \
    xx-info env && git clone --depth 1 -b $ZIPKIN_CPP_VERSION https://github.com/rnburn/zipkin-cpp-opentracing.git \
    && cd zipkin-cpp-opentracing \
    && mkdir .build && cd .build \
    && cmake $(xx-clang --print-cmake-defines) \
    -DCMAKE_PREFIX_PATH=$(xx-info sysroot)usr/local \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_STATIC_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_PLUGIN=ON \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_TESTING=OFF .. \
    && make -j$(nproc) install \
    && xx-verify /usr/local/lib/libzipkin_opentracing_plugin.so


### Build Jaeger cpp-client
FROM opentracing-cpp as jaeger-cpp-client
ARG JAEGER_CPP_VERSION=v0.9.0
ARG YAML_CPP_VERSION=yaml-cpp-0.7.0
ARG TARGETPLATFORM

# Building yaml-cpp manually because of a bug in jaeger-client-cpp that won't install it
RUN xx-info env && git clone --depth 1 -b $YAML_CPP_VERSION https://github.com/jbeder/yaml-cpp/ && \
    cd yaml-cpp && mkdir .build && cd .build && \
    cmake $(xx-clang --print-cmake-defines) \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DYAML_CPP_BUILD_TESTS=OFF \
    -DYAML_CPP_BUILD_TOOLS=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. \
    && make -j$(nproc) install \
    && xx-verify /usr/local/lib/libyaml-cpp.so

RUN git clone --depth 1 -b $JAEGER_CPP_VERSION https://github.com/jaegertracing/jaeger-client-cpp \
    && cd jaeger-client-cpp \
    && sed -i 's/hunter_add_package(yaml-cpp)/#hunter_add_package(yaml-cpp)/' CMakeLists.txt \
    && sed -i 's/yaml-cpp::yaml-cpp/yaml-cpp/' CMakeLists.txt \
    # Hunter doesn't read CMake variables, so we need to set them manually
    && printf "%s\n" "" "set(CMAKE_C_COMPILER clang)"  "set(CMAKE_CXX_COMPILER clang++)" \
    "set(CMAKE_ASM_COMPILER clang)" "set(PKG_CONFIG_EXECUTABLE  $(xx-clang --print-prog-name=pkg-config))" \
    "set(CMAKE_C_COMPILER_TARGET $(xx-clang --print-target-triple))" "set(CMAKE_CXX_COMPILER_TARGET $(xx-clang++ --print-target-triple))" \
    "set(CMAKE_ASM_COMPILER_TARGET $(xx-clang --print-target-triple))" \
    "set(CMAKE_INSTALL_PREFIX $(xx-info sysroot)usr/local)" >>  cmake/toolchain.cmake \
    && mkdir .build \
    && cd .build \
    && cmake $(xx-clang --print-cmake-defines) \
    -DCMAKE_PREFIX_PATH=$(xx-info sysroot) \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DJAEGERTRACING_BUILD_EXAMPLES=OFF \
    -DJAEGERTRACING_BUILD_CROSSDOCK=OFF \
    -DJAEGERTRACING_COVERAGE=OFF \
    -DJAEGERTRACING_PLUGIN=ON \
    -DHUNTER_CONFIGURATION_TYPES=Release \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DJAEGERTRACING_WITH_YAML_CPP=ON .. \
    && make -j$(nproc) install \
    && export HUNTER_INSTALL_DIR=$(cat _3rdParty/Hunter/install-root-dir) \
    && mkdir /hunter \
    && cp -r $HUNTER_INSTALL_DIR/lib /hunter/ \
    && cp -r $HUNTER_INSTALL_DIR/include /hunter/ \
    && mv libjaegertracing_plugin.so /usr/local/lib/libjaegertracing_plugin.so \
    && xx-verify /usr/local/lib/libjaegertracing_plugin.so


### Build dd-opentracing-cpp
FROM opentracing-cpp as dd-opentracing-cpp
ARG DATADOG_VERSION=master
ARG TARGETPLATFORM

RUN xx-info env && git clone --depth 1 -b $DATADOG_VERSION https://github.com/DataDog/dd-opentracing-cpp.git \
    && cd dd-opentracing-cpp \
    && mkdir .build && cd .build \
    && cmake $(xx-clang --print-cmake-defines) \
    -DCMAKE_PREFIX_PATH=$(xx-info sysroot) \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_TESTING=OFF .. \
    && make -j$(nproc) install \
    && ln -s /usr/local/lib/libdd_opentracing.so /usr/local/lib/libdd_opentracing_plugin.so \
    && xx-verify /usr/local/lib/libdd_opentracing_plugin.so


### Base build image for debian
FROM nginx:1.23.0 as build-nginx-debian

RUN echo "deb-src http://nginx.org/packages/mainline/debian/ bullseye nginx" >> /etc/apt/sources.list \
    && apt-get update \
    && apt-get build-dep -y nginx


### Base build image for alpine
# docker.io/library/nginx is a temporary workaround for Dependabot to see this as different from the one used in Debian
FROM docker.io/library/nginx:1.23.0-alpine AS build-nginx-alpine
RUN apk add --no-cache \
    build-base \
    pcre2-dev \
    zlib-dev


### Build nginx-opentracing modules
FROM build-nginx-${BUILD_OS} as build-nginx

COPY --from=jaeger-cpp-client /hunter /hunter
COPY . /src

RUN curl -fsSL -O https://github.com/nginx/nginx/archive/release-${NGINX_VERSION}.tar.gz \
    && tar zxf release-${NGINX_VERSION}.tar.gz \
    && cd nginx-release-${NGINX_VERSION} \
    && auto/configure \
    --with-compat \
    --add-dynamic-module=/src/opentracing \
    --with-cc-opt="-I/hunter/include" \
    --with-ld-opt="-fPIE -fPIC -Wl,-z,relro -Wl,-z,now -L/hunter/lib" \
    --with-debug \
    && make -j$(nproc) modules \
    && cp objs/ngx_http_opentracing_module.so /usr/lib/nginx/modules/


### Base image for alpine
# docker.io/library/nginx is a temporary workaround for Dependabot to see this as different from the one used in Debian
FROM docker.io/library/nginx:1.23.0-alpine as nginx-alpine
RUN apk add --no-cache libstdc++


### Base image for debian
FROM nginx:1.23.0 as nginx-debian


### Build final image
FROM nginx-${BUILD_OS} as final

COPY --from=build-nginx /usr/lib/nginx/modules/ /usr/lib/nginx/modules/
COPY --from=dd-opentracing-cpp /usr/local/lib/ /usr/local/lib/
COPY --from=jaeger-cpp-client /usr/local/lib/ /usr/local/lib/
COPY --from=zipkin-cpp-opentracing /usr/local/lib/ /usr/local/lib/
COPY --from=opentracing-cpp /usr/local/lib/ /usr/local/lib/
# gRPC doesn't seem to be used
# COPY --from=grpc /usr/local/lib/ /usr/local/lib/

RUN ldconfig /usr/local/lib/

STOPSIGNAL SIGQUIT
