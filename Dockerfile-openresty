# Dockerfile - Ubuntu Xenial
# https://github.com/openresty/docker-openresty

ARG RESTY_IMAGE_BASE="ubuntu"
ARG RESTY_IMAGE_TAG="xenial"

FROM ${RESTY_IMAGE_BASE}:${RESTY_IMAGE_TAG}

# Docker Build Arguments
ARG RESTY_VERSION="1.13.6.2"
ARG RESTY_LUAROCKS_VERSION="2.4.4"
ARG RESTY_OPENSSL_VERSION="1.1.0h"
ARG RESTY_PCRE_VERSION="8.42"
ARG RESTY_J="1"
ARG RESTY_CONFIG_OPTIONS="\
    --with-file-aio \
    --with-http_addition_module \
    --with-http_auth_request_module \
    --with-http_dav_module \
    --with-http_flv_module \
    --with-http_geoip_module=dynamic \
    --with-http_gunzip_module \
    --with-http_gzip_static_module \
    --with-http_image_filter_module=dynamic \
    --with-http_mp4_module \
    --with-http_random_index_module \
    --with-http_realip_module \
    --with-http_secure_link_module \
    --with-http_slice_module \
    --with-http_ssl_module \
    --with-http_stub_status_module \
    --with-http_sub_module \
    --with-http_v2_module \
    --with-http_xslt_module=dynamic \
    --with-ipv6 \
    --with-mail \
    --with-mail_ssl_module \
    --with-md5-asm \
    --with-pcre-jit \
    --with-sha1-asm \
    --with-stream \
    --with-stream_ssl_module \
    --with-threads \
    --add-dynamic-module=/src/opentracing \
    "
ARG RESTY_CONFIG_OPTIONS_MORE=""
ARG OPENTRACING_CPP_VERSION="v1.5.1"
ARG LUA_BRIDGE_TRACER_VERSION="9213e1b0c23a0d028093895d290c705680fbf4c5"
ARG JAEGER_VERSION="v0.4.2"
ARG LIGHTSTEP_VERSION="v0.8.1"
ARG ZIPKIN_VERSION="v0.5.2"

LABEL resty_version="${RESTY_VERSION}"
LABEL resty_luarocks_version="${RESTY_LUAROCKS_VERSION}"
LABEL resty_openssl_version="${RESTY_OPENSSL_VERSION}"
LABEL resty_pcre_version="${RESTY_PCRE_VERSION}"
LABEL resty_config_options="${RESTY_CONFIG_OPTIONS}"
LABEL resty_config_options_more="${RESTY_CONFIG_OPTIONS_MORE}"

# These are not intended to be user-specified
ARG _RESTY_CONFIG_DEPS="--with-openssl=/tmp/openssl-${RESTY_OPENSSL_VERSION} --with-pcre=/tmp/pcre-${RESTY_PCRE_VERSION}"

COPY . /src


# 1) Install apt dependencies
# 2) Download and untar OpenSSL, PCRE, and OpenResty
# 3) Build OpenResty
# 4) Cleanup

RUN DEBIAN_FRONTEND=noninteractive apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        curl \
        gettext-base \
        libgd-dev \
        libgeoip-dev \
        libncurses5-dev \
        libperl-dev \
        libreadline-dev \
        libxslt1-dev \
        make \
        perl \
        unzip \
        zlib1g-dev \
        git \
        cmake \
        lua5.1-dev \
        wget \
### Build opentracing-cpp
  && cd /tmp \
  && git clone -b ${OPENTRACING_CPP_VERSION} https://github.com/opentracing/opentracing-cpp.git \
  && cd opentracing-cpp \
  && mkdir .build && cd .build \
  && cmake -DCMAKE_BUILD_TYPE=Release \
           -DBUILD_MOCKTRACER=OFF \
           -DBUILD_STATIC_LIBS=OFF \
           -DBUILD_TESTING=OFF .. \
  && make && make install \
  && cd /tmp \
  && rm -rf opentracing-cpp \
### Build bridge tracer
  && cd /tmp \
  && git clone https://github.com/opentracing/lua-bridge-tracer.git \
  && cd lua-bridge-tracer \
  && git checkout ${LUA_BRIDGE_TRACER_VERSION} \
  && mkdir .build && cd .build \
  && cmake -DCMAKE_BUILD_TYPE=Release \
            .. \
  && make && make install \
  && cd /tmp \
  && rm -rf lua-bridge-tracer \
### Install tracers
  && wget https://github.com/jaegertracing/jaeger-client-cpp/releases/download/${JAEGER_VERSION}/libjaegertracing_plugin.linux_amd64.so -O /usr/local/lib/libjaegertracing_plugin.so \
  && wget -O - https://github.com/lightstep/lightstep-tracer-cpp/releases/download/${LIGHTSTEP_VERSION}/linux-amd64-liblightstep_tracer_plugin.so.gz | gunzip -c > /usr/local/lib/liblightstep_tracer_plugin.so \
  && wget -O - https://github.com/rnburn/zipkin-cpp-opentracing/releases/download/${ZIPKIN_VERSION}/linux-amd64-libzipkin_opentracing_plugin.so.gz | gunzip -c > /usr/local/lib/libzipkin_opentracing_plugin.so \
### Copied from https://github.com/openresty/docker-openresty/blob/master/xenial/Dockerfile
    && cd /tmp \
    && curl -fSL https://www.openssl.org/source/openssl-${RESTY_OPENSSL_VERSION}.tar.gz -o openssl-${RESTY_OPENSSL_VERSION}.tar.gz \
    && tar xzf openssl-${RESTY_OPENSSL_VERSION}.tar.gz \
    && curl -fSL https://ftp.pcre.org/pub/pcre/pcre-${RESTY_PCRE_VERSION}.tar.gz -o pcre-${RESTY_PCRE_VERSION}.tar.gz \
    && tar xzf pcre-${RESTY_PCRE_VERSION}.tar.gz \
    && curl -fSL https://openresty.org/download/openresty-${RESTY_VERSION}.tar.gz -o openresty-${RESTY_VERSION}.tar.gz \
    && tar xzf openresty-${RESTY_VERSION}.tar.gz \
    && cd /tmp/openresty-${RESTY_VERSION} \
    && ./configure -j${RESTY_J} ${_RESTY_CONFIG_DEPS} ${RESTY_CONFIG_OPTIONS} ${RESTY_CONFIG_OPTIONS_MORE} \
    && make -j${RESTY_J} \
    && make -j${RESTY_J} install \
    && cd /tmp \
    && rm -rf \
        openssl-${RESTY_OPENSSL_VERSION} \
        openssl-${RESTY_OPENSSL_VERSION}.tar.gz \
        openresty-${RESTY_VERSION}.tar.gz openresty-${RESTY_VERSION} \
        pcre-${RESTY_PCRE_VERSION}.tar.gz pcre-${RESTY_PCRE_VERSION} \
    && curl -fSL https://github.com/luarocks/luarocks/archive/${RESTY_LUAROCKS_VERSION}.tar.gz -o luarocks-${RESTY_LUAROCKS_VERSION}.tar.gz \
    && tar xzf luarocks-${RESTY_LUAROCKS_VERSION}.tar.gz \
    && cd luarocks-${RESTY_LUAROCKS_VERSION} \
    && ./configure \
        --prefix=/usr/local/openresty/luajit \
        --with-lua=/usr/local/openresty/luajit \
        --lua-suffix=jit-2.1.0-beta3 \
        --with-lua-include=/usr/local/openresty/luajit/include/luajit-2.1 \
    && make build \
    && make install \
    && cd /tmp \
    && rm -rf /src \
    && rm -rf luarocks-${RESTY_LUAROCKS_VERSION} luarocks-${RESTY_LUAROCKS_VERSION}.tar.gz \
    && DEBIAN_FRONTEND=noninteractive apt-get autoremove -y \
    && ln -sf /dev/stdout /usr/local/openresty/nginx/logs/access.log \
    && ln -sf /dev/stderr /usr/local/openresty/nginx/logs/error.log \
    && ldconfig

# Add additional binaries into PATH for convenience
ENV PATH=$PATH:/usr/local/openresty/luajit/bin:/usr/local/openresty/nginx/sbin:/usr/local/openresty/bin

# Copy nginx configuration files
ADD https://raw.githubusercontent.com/openresty/docker-openresty/master/nginx.conf /usr/local/nginx/conf/nginx.conf
ADD https://raw.githubusercontent.com/openresty/docker-openresty/master/nginx.vh.default.conf /etc/nginx/conf.d/default.conf

CMD ["/usr/local/openresty/bin/openresty", "-g", "daemon off;"]
