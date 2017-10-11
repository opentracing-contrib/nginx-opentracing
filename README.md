nginx-opentracing
-----------------

Enable requests served by nginx for distributed tracing via [The OpenTracing Project](opentracing.io).

Dependencies
------------
- The [C++ OpenTracing Library](https://github.com/opentracing/opentracing-cpp)
- A C++ OpenTracing Tracer. It currently works with either
[Zipkin](https://github.com/rnburn/zipkin-cpp-opentracing) or
[LightStep](https://github.com/lightstep/lightstep-tracer-cpp).
- Source for [Nginx 1.9.13 or later](http://nginx.org/).

Dependencies
------------
A [Dockerfile](docker/Dockerfile) is provided to support using nginx with OpenTracing
in a manner analogous to the [nginx Docker image](https://hub.docker.com/_/nginx/). 
See the [here](example/) for examples of how to use it.

Building
--------
```
$ tar zxvf nginx-1.9.x.tar.gz
$ cd nginx-1.9.x
$ export NGINX_OPENTRACING_VENDOR="ZIPKIN" # or export NGINX_OPENTRACING_VENDOR="LIGHTSTEP"
$ ./configure --add-dynamic-module=/absolute/path/to/nginx-opentracing/opentracing \
              # To enable tracing with Zipkin
              --add-dynamic-module=/absolute/path/to/nginx-opentracing/zipkin \  
              # To enable tracing with LightStep
              --add-dynamic-module=/absolute/path/to/nginx-opentracing/lightstep
$ make && sudo make install
```


Getting Started
---------------
```
# Load the OpenTracing dynamic module.
load_module modules/ngx_http_opentracing_module.so;

# Load a vendor OpenTracing dynamic module.
# For example,
#   load_module modules/ngx_http_lightstep_module.so;
# or
#   load_module modules/ngx_http_zipkin_module.so;

http {
  # Configure your vendor's tracer.
  # For example,
  #     lightstep_access_token ACCESSTOKEN;
  #     ....
  # or
  #     zipkin_collector_host localhost;
  #     ...

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

See [Tutorial](doc/Tutorial.md) for a more complete example, [Reference](doc/Directives.md)
for a list of available OpenTracing-related directives, and [LightStep](lightstep/doc/Directives.md)
and [Zipkin](zipkin/doc/Directives.md) for a list of vendor tracing directives.

