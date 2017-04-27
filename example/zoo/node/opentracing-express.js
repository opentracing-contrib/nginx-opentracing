// Modified from github.com/Clever/tracing-middleware

const opentracing = require('opentracing');
const url = require('url');

exports.middleware = function middleware(options) {
  if (typeof options === 'undefined') {
    options = {};
  }
  if (options.tracer) {
    opentracing.initGlobalTracer(options.tracer);
  } else {
    opentracing.initGlobalTracer();
  }

  return (req, res, next) => {
    const tracer = opentracing.globalTracer();
    const wireCtx =
        tracer.extract(opentracing.FORMAT_HTTP_HEADERS, req.headers);
    const pathname = url.parse(req.url).pathname;
    const span = tracer.startSpan(pathname, { childOf: wireCtx });
    span.logEvent('request_received');

    // include some useful tags on the trace
    span.setTag('http.method', req.method);
    span.setTag('span.kind', 'server');
    span.setTag('http.url', req.url);
    span.setTag('component', 'express');

    // include trace ID in headers so that we can debug slow requests we see in
    // the browser by looking up the trace ID found in response headers
    const responseHeaders = {};
    tracer.inject(span, opentracing.FORMAT_TEXT_MAP, responseHeaders);
    Object.keys(responseHeaders)
        .forEach(key => res.setHeader(key, responseHeaders[key]));

    // add the span to the request object for handlers to use
    Object.assign(req, { span });

    // finalize the span when the response is completed
    const endOld = res.end;
    res.end = function() {
      span.logEvent('request_finished');
      // Route matching often happens after the middleware is run. Try changing
      // the operation name
      // to the route matcher.
      const opName = (req.route && req.route.path) || pathname;
      span.setOperationName(opName);
      span.setTag('http.status_code', res.statusCode);
      if (res.statusCode >= 500) {
        span.setTag('error', true);
        span.setTag('sampling.priority', 1);
      }
      span.finish();
      endOld.apply(this, arguments);
    };

    // Create a span for any rendered templates.
    const renderOld = res.render;
    res.render = function(view, options, callback) {
      let done = callback;
      let opts = options || {};
      const req = this.req;
      const self = this;
      if (typeof options === 'function') {
        done = options;
        opts = {};
      }

      done = done || function(err, str) {
        if (err) {
          return req.next(err);
        }
        self.send(str);
      };

      const templateSpan = tracer.startSpan(view, { childOf: span });
      templateSpan.setTag('component', 'express-template');
      const doneWrap = function(err, str) {
        if (err) {
          templateSpan.log({
            event: 'error',
            'error.object': err,
          });
          templateSpan.setTag('error', true);
        }
        templateSpan.finish();
        done.apply(this, arguments);
      };

      renderOld.apply(this, [view, opts, doneWrap]);
    };
    next();
  };
};
