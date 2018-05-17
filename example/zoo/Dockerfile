FROM ubuntu:17.10

WORKDIR /app
ADD . /app
RUN set -x \
## install nodejs and node dependencies
  && apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
              python \
              curl \
              ca-certificates \
              gnupg2 \
              build-essential \
  && mkdir /node-latest-install \
  && cd /node-latest-install \
  && curl -sL https://deb.nodesource.com/setup_8.x -o nodesource_setup.sh \
  && bash nodesource_setup.sh \
  && apt-get install --no-install-recommends --no-install-suggests -y \
              nodejs \
  && cd /app \
  && npm install . \
## set up directories
  && mkdir /app/data \
  && chmod a+rw /app/data \
  && mkdir /app/data/images \
  && chmod a+rw /app/data/images


EXPOSE 80
CMD ["/app/start-backend.sh"]
