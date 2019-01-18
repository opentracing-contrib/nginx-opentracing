FROM ubuntu:18.04

ARG OPENTRACING_CPP_VERSION=v1.5.1
ARG NGINX_VERSION=1.13.12

RUN set -x \
  && apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
              build-essential \
              gettext \
              cmake \
              git \
              gnupg2 \
              software-properties-common \
              curl \
              python3 \
              jq \
              ca-certificates \
              wget \
              libpcre3 libpcre3-dev \
              zlib1g-dev \
              gcc-8 \
              g++-8 \
### Use gcc-8 (the default gcc has this problem when using with address sanitizer:
### https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84428)
  && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8

### Build opentracing-cpp
RUN cd / \
  && git clone -b $OPENTRACING_CPP_VERSION https://github.com/opentracing/opentracing-cpp.git \
  && cd opentracing-cpp \
  && mkdir .build && cd .build \
  && cmake -DCMAKE_BUILD_TYPE=Debug \
           -DBUILD_TESTING=OFF .. \
  && make && make install

COPY ./opentracing /opentracing

### Build nginx
RUN apt-get install -y libssl-dev
RUN cd / \
  && wget -O nginx-release-${NGINX_VERSION}.tar.gz https://github.com/nginx/nginx/archive/release-${NGINX_VERSION}.tar.gz \
  && tar zxf nginx-release-${NGINX_VERSION}.tar.gz \
  && cd /nginx-release-${NGINX_VERSION} \
  # Temporarily disable leak sanitizer to get around false positives in build
  && export ASAN_OPTIONS=detect_leaks=0 \
  && export CFLAGS="-Wno-error" \
  && auto/configure \
      --with-http_auth_request_module \
      --with-compat \
      --with-debug \
      # enables grpc
      --with-http_v2_module \
      --add-dynamic-module=/opentracing \
  && make && make install

CMD ["/usr/local/nginx/sbin/nginx", "-g", "daemon off;"]
