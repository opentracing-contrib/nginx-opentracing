set -x
apt-get update
apt-get install --no-install-recommends --no-install-suggests -y \
            build-essential \
            python3 \
            ca-certificates \
            python3-dev \
            python-setuptools \
            python3-pip \
            curl \
            software-properties-common
pip3 install setuptools 
pip3 install docker
pip3 install docker-compose
