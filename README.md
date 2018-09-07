nginx-opentracing
-----------------

Enable requests served by nginx for distributed tracing via [The OpenTracing Project](http://opentracing.io).

Dependencies
------------
- The [C++ OpenTracing Library](https://github.com/opentracing/opentracing-cpp)
- A C++ OpenTracing Tracer. It currently works with
[Jaeger](https://github.com/jaegertracing/cpp-client),
[Zipkin](https://github.com/rnburn/zipkin-cpp-opentracing),
[LightStep](https://github.com/lightstep/lightstep-tracer-cpp), or
[Datadog](https://github.com/DataDog/dd-opentracing-cpp/).
- Source for [Nginx 1.9.13 or later](http://nginx.org/).

Installation
------------
For `linux-x86_64`, pre-compiled binaries are provided for the supported versions of NGINX.
These can be dropped into existing NGINX installations provided that NGINX was compiled with
the `--with-compat` option. See [example/trivial/ubuntu-x86_64](example/trivial/ubuntu-x86_64) for an
example of how to set it up.

Otherwise, nginx-opentracing can be used from the [Docker image](https://github.com/opentracing-contrib/nginx-opentracing#docker) or [built from source](https://github.com/opentracing-contrib/nginx-opentracing#building-from-source).

Getting Started
---------------
First, write a configuration for the tracer used. Below's an example of what
a Jaeger configuration might look like:

/etc/jaeger-nginx-config.json
```
{
  "service_name": "nginx",
  "sampler": {
    "type": "const",
    "param": 1
  },
  "reporter": {
    "localAgentHostPort": "jaeger:6831"
  },
  "headers": {
    "jaegerDebugHeader": "jaeger-debug-id",
    "jaegerBaggageHeader": "jaeger-baggage",
    "traceBaggageHeaderPrefix": "uberctx-",
  },
  "baggage_restrictions": {
    "denyBaggageOnInitializationFailure": false,
    "hostPort": ""
  }
}
```

See the vendor documentation for details on what options are available.

You can then set up NGINX for distributed tracing by adding the following to
nginx.conf:
```
# Load the OpenTracing dynamic module.
load_module modules/ngx_http_opentracing_module.so;

http {
  # Load a vendor tracer
  opentracing_load_tracer /usr/local/lib/libjaegertracing_plugin.so /etc/jaeger-nginx-config.json;

  # or 
  #   opentracing_load_tracer /usr/local/lib/liblightstep_tracer_plugin.so /path/to/config;
  # or 
  #   opentracing_load_tracer /usr/local/lib/libzipkin_opentracing_plugin.so /path/to/config;
  # or
  #   opentracing_load_tracer /usr/local/lib/libdd_opentracing_plugin.so /path/to/config;

  # Enable tracing for all requests.
  opentracing on;

  # Optionally, set additional tags.
  opentracing_tag http_user_agent $http_user_agent;

  upstream backend {
    server app-service:9001;
  }

  location ~ {
    # The operation name used for spans defaults to the name of the location
    # block, but you can use this directive to customize it.
    opentracing_operation_name $uri;

    # Propagate the active span context upstream, so that the trace can be
    # continued by the backend.
    # See http://opentracing.io/documentation/pages/api/cross-process-tracing.html
    opentracing_propagate_context;

    proxy_pass http://backend;
  }
}
```

See [Tutorial](doc/Tutorial.md) for a more complete example,
[Reference](doc/Reference.md) for a list of available OpenTracing-related
directives.

Docker
------
A docker image `opentracing/nginx-opentracing` is provided to support using nginx with OpenTracing
in a manner analogous to the [nginx Docker image](https://hub.docker.com/_/nginx/). 
See [here](example/) for examples of how to use it.

Additionally, custom images can be built by running

```bash
docker build \
       -t opentracing-contrib/nginx-opentracing:latest \
       .
```

and arguments to weak the versions used can be provided with

```bash
docker build \
       -t opentracing-contrib/nginx-opentracing:latest \
       --build-arg OPENTRACING_CPP_VERSION=master \
       .
```

Other build arguments

* `OPENTRACING_CPP_VERSION`
* `ZIPKIN_CPP_VERSION`
* `LIGHTSTEP_VERSION`
* `JAEGER_CPP_VERSION`
* `GRPC_VERSION`
* `NGINX_OPENTRACING_VERSION`


Building From Source
--------------------
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
wget https://github.com/jaegertracing/jaeger-client-cpp/releases/download/v0.4.1/libjaegertracing_plugin.linux_amd64.so -O /usr/local/lib/libjaegertracing_plugin.so

# LightStep
wget -O - https://github.com/lightstep/lightstep-tracer-cpp/releases/download/v0.7.1/linux-amd64-liblightstep_tracer_plugin.so.gz | gunzip -c > /usr/local/lib/liblightstep_tracer_plugin.so

# Zipkin
wget -O - https://github.com/rnburn/zipkin-cpp-opentracing/releases/download/v0.4.0/linux-amd64-libzipkin_opentracing_plugin.so.gz | gunzip -c > /usr/local/lib/libzipkin_opentracing_plugin.so

# Datadog
wget -O - https://github.com/DataDog/dd-opentracing-cpp/releases/download/v0.2.4/linux-amd64-libdd_opentracing_plugin.so.gz | gunzip -c > /usr/local/lib/libdd_opentracing_plugin.so

```

