nginx-opentracing
-----------------

Enable requests served by nginx for distributed tracing via [The OpenTracing Project](opentracing.io).

Dependencies
------------
- The [C++ OpenTracing Library](https://github.com/opentracing/opentracing-cpp)
- A C++ OpenTracing Tracer. It currently works with
[Jaeger](https://github.com/jaegertracing/cpp-client),
[Zipkin](https://github.com/rnburn/zipkin-cpp-opentracing), or
[LightStep](https://github.com/lightstep/lightstep-tracer-cpp).
- Source for [Nginx 1.9.13 or later](http://nginx.org/).

Docker
------------
A docker image `rnburn/nginx-opentracing` is provided to support using nginx with OpenTracing
in a manner analogous to the [nginx Docker image](https://hub.docker.com/_/nginx/). 
See [here](example/) for examples of how to use it.

Building
--------
```
$ tar zxvf nginx-1.9.x.tar.gz
$ cd nginx-1.9.x
$ ./configure --add-dynamic-module=/absolute/path/to/nginx-opentracing/opentracing
$ make && sudo make install
```

You will also need to install a C++ tracer for either [Jaeger](https://github.com/jaegertracing/jaeger-client-cpp), [LightStep](
https://github.com/lightstep/lightstep-tracer-cpp), or [Zipkin](https://github.com/rnburn/zipkin-cpp-opentracing). For linux x86-64, portable binary plugins are available:
```
# Jaeger
wget https://github.com/jaegertracing/jaeger-client-cpp/releases/download/v0.4.0/libjaegertracing_plugin.linux_amd64.so -O /usr/local/lib/libjaegertracing_plugin.so

# LightStep
wget -O - https://github.com/lightstep/lightstep-tracer-cpp/releases/download/v0.7.0/linux-amd64-liblightstep_tracer_plugin.so.gz | gunzip -c > /usr/local/lib/liblightstep_tracer_plugin.so

# Zipkin
wget -O - https://github.com/rnburn/zipkin-cpp-opentracing/releases/download/v0.3.1/linux-amd64-libzipkin_opentracing_plugin.so.gz  gunzip -c > /usr/local/lib/libzipkin_opentracing_plugin.so

```

Getting Started
---------------
```
# Load the OpenTracing dynamic module.
load_module modules/ngx_http_opentracing_module.so;

# Load a vendor OpenTracing dynamic module.
# For example,
#   load_module modules/ngx_http_jaeger_module.so;
# or
#   load_module modules/ngx_http_zipkin_module.so;
# or
#   load_module modules/ngx_http_lightstep_module.so;

http {
  # Configure your vendor's tracer.
  # For example,
  #     jaeger_service_name my-nginx-server;
  #     ...
  # or
  #     zipkin_collector_host localhost;
  #     ...
  # or
  #     lightstep_access_token ACCESSTOKEN;
  #     ....

  # Enable tracing for all requests.
  opentracing on;

  # Optionally, set additional tags.
  opentracing_tag http_user_agent $http_user_agent;

  location ~ \.php$ {
    # The operation name used for spans defaults to the name of the location
    # block, but you can use this directive to customize it.
    opentracing_operation_name $uri;

    fastcgi_pass 127.0.0.1:1025;
  }
}
```

See [Tutorial](doc/Tutorial.md) for a more complete example,
[Reference](doc/Directives.md) for a list of available OpenTracing-related
directives.

