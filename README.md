nginx-opentracing
-----------------

Enable requests served by nginx for distributed tracing via [The OpenTracing Project](opentracing.io).

Dependencies
------------
- nginx-opentracing currently only works with LightStep's C++ tracer. See 
[here](http://lightstep.com/#request-access) to request access and [here](https://github.com/lightstep/lightstep-tracer-cpp) to install.
- Source for [Nginx 1.0.x](http://nginx.org/).

Building
--------
```
$ tar zxvf nginx-1.0.x.tar.gz
$ cd nginx-1.0.x
$ ./configure --add-module=/absolute/path/to/nginx-opentracing
$ make && sudo make install
```
