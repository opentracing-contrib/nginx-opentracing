Demonstates how to set up nginx-opentracing and Jaeger for `linux-x86_64` using
prebuilt binary images.

NGINX-1.14.0 is installed for Ubuntu following the instructions [here](https://www.nginx.com/resources/wiki/start/topics/tutorials/install/#official-debian-ubuntu-packages).

nginx-opentracing is installed into NGINX's module directory with
```bash
cd /usr/lib/nginx/modules
wget -O - https://github.com/opentracing-contrib/nginx-opentracing/releases/download/v0.4.0/linux-amd64-nginx-1.14.0-ngx_http_module.so.tgz \
      | tar zxf -
```

and Jaeger is installed with
```bash
cd /usr/local/lib
wget -O libjaegertracing_plugin.so https://github.com/jaegertracing/jaeger-client-cpp/releases/download/v0.4.1/libjaegertracing_plugin.linux_amd64.so
```

Use these commands to run:
```bash
docker-compose up
curl localhost:8080
```
Visit http://localhost:16686 to view the traces in Jaeger.
