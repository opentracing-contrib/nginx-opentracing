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
	\
# reset apt-mark's "manual" list so that "purge --auto-remove" will remove all build dependencies
# (which is done after we install the built packages so we don't have to redownload any overlapping dependencies)
	&& apt-mark showmanual | xargs apt-mark auto > /dev/null \
	&& { [ -z "$savedAptMark" ] || apt-mark manual $savedAptMark; } \
	\
## Build opentracing-cpp
  && git clone https://github.com/opentracing/opentracing-cpp.git \
  && cd opentracing-cpp \
  && mkdir .build && cd .build \
  && cmake -DCMAKE_BUILD_TYPE=Release .. \
  && make && make install \
  && cd "$tempDir" \
## Build zipkin-cpp-opentracing
  && apt-get --no-install-recommends --no-install-suggests -y install libcurl4-gnutls-dev \
  && git clone https://github.com/rnburn/zipkin-cpp-opentracing.git \
  && cd zipkin-cpp-opentracing \
  && mkdir .build && cd .build \
  && cmake -DBUILD_SHARED_LIBS=1 -DCMAKE_BUILD_TYPE=Release .. \
  && make && make install \
  && cd "$tempDir" \
### Build gRPC
 && git clone -b v1.4.x https://github.com/grpc/grpc \
 && cd grpc \
 && git submodule update --init \
 && make HAS_SYSTEM_PROTOBUF=false && make install \
# && mkdir .build && cd .build \
# && cmake -DCMAKE_BUILD_TYPE=Release -DgRPC_ZLIB_PROVIDER=package .. \
 && make && make install \
 && cd "$tempDir" \
### Build lightstep-tracer-cpp
# && apt-get install --no-install-recommends --no-install-suggests -y \
#              libprotobuf-dev \
#              protobuf-compiler \
 && git clone https://github.com/lightstep/lightstep-tracer-cpp.git \
 && cd lightstep-tracer-cpp \
 && mkdir .build && cd .build \
 && cmake -DBUILD_SHARED_LIBS=1 -DCMAKE_BUILD_TYPE=Release .. \
 && make && make install \
 && cd "$tempDir" \
### Get nginx-opentracing source
  && git clone -b dymod https://github.com/rnburn/nginx-opentracing.git \
## Build nginx-opentracing modules
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
  && make modules \
  && cp objs/ngx_http_opentracing_module.so $NGINX_MODULES_PATH/ \
  && cp objs/ngx_http_zipkin_module.so $NGINX_MODULES_PATH/ \
  && cp objs/ngx_http_lightstep_module.so $NGINX_MODULES_PATH/ \
	# if we have leftovers from building, let's purge them (including extra, unnecessary build deps)
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
