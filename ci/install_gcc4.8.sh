#!/bin/bash

set -e
apt-get update 
apt-get install --no-install-recommends --no-install-suggests -y \
          software-properties-common
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get update
apt-get install --no-install-recommends --no-install-suggests -y \
          gcc-4.8 \
          g++-4.8
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 5
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 5
