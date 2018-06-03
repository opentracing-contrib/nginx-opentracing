FROM ubuntu:17.10

WORKDIR /app
ADD . /app
ENV GOPATH="/go"
RUN set -x \
  && apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
              golang \
              ca-certificates \
              git \
  && mkdir /go \
  && go get github.com/opentracing/opentracing-go \
  && go get github.com/openzipkin/zipkin-go-opentracing \
  && go build -o /app/server go/server.go

EXPOSE 9001
CMD ["/app/server", "-collector_host", "zipkin", "-collector_port", "9411"]
