#!/bin/bash

set -x
set -e

# workaround to install docker-compose v1
sudo curl -L "https://github.com/docker/compose/releases/download/1.29.2/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose

docker version
docker-compose version

pyenv versions
pyenv local 3.10.3
python --version
pip --version
pip install --upgrade pip
pip install --upgrade setuptools
pip install -r test/requirements.ci.txt
