FROM ubuntu:17.10

ARG OPENTRACING_CPP_VERSION=v1.4.0

RUN set -x \
  && apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
              build-essential \
              gettext \
              cmake \
              git \
              curl \
              python3 \
              jq \
              ca-certificates \
              wget \
              libpcre3 libpcre3-dev \
              zlib1g-dev \
  && mkdir /src \
### Build opentracing-cpp
  && cd /src \
 && git clone -b $OPENTRACING_CPP_VERSION https://github.com/opentracing/opentracing-cpp.git \
  && cd opentracing-cpp \
  && mkdir .build && cd .build \
  && cmake -DCMAKE_BUILD_TYPE=Debug \
           -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer -fsanitize=address" \
           -DCMAKE_SHARED_LINKER_FLAGS="-fno-omit-frame-pointer -fsanitize=address" \
           -DCMAKE_EXE_LINKER_FLAGS="-fno-omit-frame-pointer -fsanitize=address" \
           -DBUILD_TESTING=OFF .. \
  && make && make install
