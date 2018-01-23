# Nginx opentracing

## Build

To build the Docker image run 

´´´bash
docker build \
       -t opentracing-contrib/nginx-opentracing:latest \
       .
´´´

Custom arguments

´´´bash
docker build \
       -t opentracing-contrib/nginx-opentracing:latest \
       --build-arg OPENTRACING_CPP_VERSION=master \
       .
´´´

Other build arguments

* ´OPENTRACING_CPP_VERSION´
* ´ZIPKIN_CPP_VERSION´
* ´LIGHTSTEP_VERSION´
* ´JAEGER_CPP_VERSION´
* ´GRPC_VERSION´
* ´NGINX_OPENTRACING_VERSION´

