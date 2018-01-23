FROM nginx
RUN set -x \
# install nginx-opentracing package dependencies
  && apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
              libcurl4-openssl-dev \
              libprotobuf-dev \
              protobuf-compiler \
# save list of currently-installed packages so build dependencies can be cleanly removed later
	&& savedAptMark="$(apt-mark showmanual)" \
# new directory for storing sources and .deb files
	&& tempDir="$(mktemp -d)" \
	&& chmod 777 "$tempDir" \
			\
# (777 to ensure APT's "_apt" user can access it too)
## Build OpenTracing package and tracers
  && apt-get install --no-install-recommends --no-install-suggests -y \
              build-essential \
              cmake \
              git \
              ca-certificates \
              pkg-config \
              wget \
              golang \
              libz-dev \
              automake \
              autogen \
              autoconf \
              libtool \
# reset apt-mark's "manual" list so that "purge --auto-remove" will remove all build dependencies
# (which is done after we install the built packages so we don't have to redownload any overlapping dependencies)
	&& apt-mark showmanual | xargs apt-mark auto > /dev/null \
	&& { [ -z "$savedAptMark" ] || apt-mark manual $savedAptMark; } \
	\
  && cd "$tempDir" \
## Build opentracing-cpp
  && git clone -b v1.2.0 https://github.com/opentracing/opentracing-cpp.git \
  && cd opentracing-cpp \
  && mkdir .build && cd .build \
  && cmake -DCMAKE_BUILD_TYPE=Release \
           -DBUILD_TESTING=OFF .. \
  && make && make install \
  && cd "$tempDir" \
## Build zipkin-cpp-opentracing
  && apt-get --no-install-recommends --no-install-suggests -y install libcurl4-gnutls-dev \
  && git clone -b v0.2.0 https://github.com/rnburn/zipkin-cpp-opentracing.git \
  && cd zipkin-cpp-opentracing \
  && mkdir .build && cd .build \
  && cmake -DBUILD_SHARED_LIBS=1 -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF .. \
  && make && make install \
  && cd "$tempDir" \
### Build Jaeger cpp-client
 && git clone -b v0.1.0 https://github.com/jaegertracing/cpp-client.git jaeger-cpp-client \
 && cd jaeger-cpp-client \
 && mkdir .build && cd .build \
 && cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_TESTING=OFF \
          -DJAEGERTRACING_WITH_YAML_CPP=OFF .. \
 && make && make install \
 && export HUNTER_INSTALL_DIR=$(cat _3rdParty/Hunter/install-root-dir) \
 && cd "$tempDir" \
### Build gRPC
 && git clone -b v1.4.x https://github.com/grpc/grpc \
 && cd grpc \
 && git submodule update --init \
 && make HAS_SYSTEM_PROTOBUF=false && make install \
 && make && make install \
 && cd "$tempDir" \
### Build lightstep-tracer-cpp
 && git clone -b v0.6.1 https://github.com/lightstep/lightstep-tracer-cpp.git \
 && cd lightstep-tracer-cpp \
 && mkdir .build && cd .build \
 && cmake -DBUILD_SHARED_LIBS=1 -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF .. \
 && make && make install \
 && cd "$tempDir" \
### Build nginx-opentracing modules
  && git clone https://github.com/opentracing-contrib/nginx-opentracing.git \
  && NGINX_VERSION=`nginx -v 2>&1` && NGINX_VERSION=${NGINX_VERSION#*nginx/} \
  && echo "deb-src http://nginx.org/packages/mainline/debian/ stretch nginx" >> /etc/apt/sources.list \
  && apt-get update \
  && apt-get build-dep -y nginx=${NGINX_VERSION} \
  && wget -O nginx-release-${NGINX_VERSION}.tar.gz https://github.com/nginx/nginx/archive/release-${NGINX_VERSION}.tar.gz \
  && tar zxf nginx-release-${NGINX_VERSION}.tar.gz \
  && cd nginx-release-${NGINX_VERSION} \
  && NGINX_MODULES_PATH=$(nginx -V 2>&1 | grep -oP "modules-path=\K[^\s]*") \
  && auto/configure \
        --with-compat \
        --add-dynamic-module=${tempDir}/nginx-opentracing/opentracing \
        --add-dynamic-module=${tempDir}/nginx-opentracing/zipkin \
        --add-dynamic-module=${tempDir}/nginx-opentracing/lightstep \
        --add-dynamic-module=${tempDir}/nginx-opentracing/jaeger \
        --with-cc-opt="-I$HUNTER_INSTALL_DIR/include" \
        --with-ld-opt="-L$HUNTER_INSTALL_DIR/lib" \
  && make modules \
  && cp objs/ngx_http_opentracing_module.so $NGINX_MODULES_PATH/ \
  && cp objs/ngx_http_zipkin_module.so $NGINX_MODULES_PATH/ \
  && cp objs/ngx_http_lightstep_module.so $NGINX_MODULES_PATH/ \
  && cp objs/ngx_http_jaeger_module.so $NGINX_MODULES_PATH/ \
	# if we have leftovers from building, let's purge them (including extra, unnecessary build deps)
  && rm -rf $HOME/.hunter \
  && if [ -n "$tempDir" ]; then \
  	apt-get purge -y --auto-remove \
  	&& rm -rf "$tempDir" /etc/apt/sources.list.d/temp.list; \
  fi

# forward request and error logs to docker log collector
RUN ln -sf /dev/stdout /var/log/nginx/access.log \
	&& ln -sf /dev/stderr /var/log/nginx/error.log

EXPOSE 80

STOPSIGNAL SIGTERM

CMD ["nginx", "-g", "daemon off;"]
