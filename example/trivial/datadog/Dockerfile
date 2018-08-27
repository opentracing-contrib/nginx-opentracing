FROM ubuntu:18.04 
# 17.10 doesn't come with latest golang.

WORKDIR /app
ADD . /app
ENV GOPATH="/go"
RUN set -x \
  && apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
  golang-go \
  ca-certificates \
  git \
  && mkdir /go \
  && go get github.com/opentracing/opentracing-go \
  && go get gopkg.in/DataDog/dd-trace-go.v1/ddtrace \
  && go build -o /app/server go/server.go

EXPOSE 9001
CMD ["/app/server"]
