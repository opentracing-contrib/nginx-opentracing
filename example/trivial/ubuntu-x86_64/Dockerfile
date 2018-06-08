FROM ubuntu:18.04

WORKDIR /app
ADD . /app
ENV GOPATH="/app/go"
RUN set -x \
  && apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
              golang \
              ca-certificates \
              git \
              curl \
  && export PATH=$PATH:$GOPATH/bin \
  && mkdir -p $GOPATH/bin \
  && curl https://glide.sh/get | sh \
  && cd go/src/hello-backend \
  && glide up \
  && glide install \
  && go build -o /app/server server.go

EXPOSE 9001
CMD ["/app/server", "-collector_host", "jaeger", "-collector_port", "6831"]
