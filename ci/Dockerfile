FROM docker:dind

RUN set -x \
  && apk add --no-cache bash \
                        python3 \
  && python3 -m ensurepip \
  && rm -r /usr/lib/python*/ensurepip \
  && pip3 install --upgrade pip setuptools \
  && pip3 install docker \
  && pip3 install docker-compose

# RUN set -x \
#   && apt-get update \
#   && apt-get install --no-install-recommends --no-install-suggests -y \
#               build-essential \
#               python3 \
#               ca-certificates \
#               python3-dev \
#               python-setuptools \
#               python3-pip \
#               curl \
#               software-properties-common \
# Install docker python
  # && pip3 install setuptools \
  # && pip3 install docker \
  # && pip3 install docker-compose
# # Install docker
#   && curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add - \
#   && add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" \
#   && apt-get update \
#   && apt-cache policy docker-ce \
#   && apt-get install -y docker-ce \
