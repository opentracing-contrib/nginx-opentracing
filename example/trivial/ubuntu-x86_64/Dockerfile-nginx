FROM ubuntu:18.04

RUN set -x \
# Install package dependencies 
  && apt-get update \
  && apt-get install --no-install-recommends --no-install-suggests -y \
     wget \
     gnupg2 \
     software-properties-common \
     ca-certificates \
# Install NGINX following these instructions
# https://www.nginx.com/resources/wiki/start/topics/tutorials/install/#official-debian-ubuntu-packages
  && wget -O - http://nginx.org/keys/nginx_signing.key | apt-key add -  \
  && apt-add-repository "deb http://nginx.org/packages/ubuntu/ bionic nginx" \
  && apt-get update \
  && apt-get install nginx=1.14.0-1~bionic \
# Install nginx-opentracing into NGINX's module directory
  && cd /usr/lib/nginx/modules \
  && wget -O - https://github.com/opentracing-contrib/nginx-opentracing/releases/download/v0.4.0/linux-amd64-nginx-1.14.0-ngx_http_module.so.tgz \
      | tar zxf - \
# Install Jaeger
  && cd /usr/local/lib \
  && wget -O libjaegertracing_plugin.so https://github.com/jaegertracing/jaeger-client-cpp/releases/download/v0.4.1/libjaegertracing_plugin.linux_amd64.so

CMD ["nginx", "-g", "daemon off;"]
