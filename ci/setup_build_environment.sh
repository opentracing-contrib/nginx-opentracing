#!/bin/bash

set -e
apt-get update 
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
                zlib1g-dev \
