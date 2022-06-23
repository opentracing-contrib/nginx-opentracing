#!/bin/bash

set -x
set -e

pyenv versions
pyenv local 3.10.3
python --version
pip --version
pip install --upgrade pip
pip install --upgrade setuptools
pip install -r test/requirements.ci.txt
