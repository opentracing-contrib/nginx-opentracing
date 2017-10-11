FROM rnburn/nginx-opentracing

WORKDIR /app
ADD . /app
ENV GOPATH="/go"
RUN set -x \
  && apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
              golang \
              git \
  && mkdir /go \
  && go get github.com/opentracing/opentracing-go \
  && go get github.com/openzipkin/zipkin-go-opentracing \
  && go build -o /app/server go/server.go

EXPOSE 80
CMD ["/app/start.sh"]
