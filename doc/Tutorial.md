Virtual Zoo
===========

In this tutorial, we'll enable an application for OpenTracing and use the
tracing data to guide us in making several optimizations. The application we're
going to work with is a virtual zoo. Users can admit new animals into the zoo
by filling out a form and submitting a profile picture. 

![alt text](data/Admit.png "Admit New Animal")

Our application keeps track of its members with a database and renders a splash
page with thumbnail pictures of all the animals organized into a table.

![alt text](data/Splash.png "Splash Page")

The application uses NGINX to load-balance between multiple Node.js backends
and serve static content. When processing a new admittance, a Node.js server
writes the profile data to a shared sqlite database and resizes the profile
picture to a common thumbnail size. Here's what the NGINX configuration looks 
like:

```
events {}

http {
  upstream backend {
    server node-server1:3000;
    server node-server2:3000;
    server node-server3:3000;
  }

  server {
    listen 8080;
    server_name localhost;

    # Displays the zoo's splash page.
    location = / {
      # Don't pass the body so that we can handle internal redirects
      # from `/upload/animal`
      proxy_set_body             off;

      # The backend returns a table of thumbnail profile pictures for every
      # animal in the zoo.
      proxy_pass http://backend;
    }

    # Display the profile for a specific animal in the zoo.
    location = /animal {
      proxy_pass http://backend;
    }

    # Admit a new animal.
    location = /upload/animal {
      limit_except POST { deny all; }

      # The backend will add the new animal into the database, create a new
      # thumbnail sized profile picture, and transform the full-sized profile
      # picture, if necessary, to be in the jpeg format.
      proxy_pass http://backend;
      
      # Redirect to the spash page if the animal was successfully admitted.
      proxy_intercept_errors on;
      error_page 301 302 303 =200 /;
    }

    location / {
      root www; 
    }

    location ~ \.jpg$ {
      # Set the root directory to where the Node.js backend uploads profile 
      # images.
      include image_params;
    }
  }
}
```

Enabling OpenTracing for NGINX
------------------------------

We can tell NGINX to trace every request by adding these two lines to
`nginx.conf`:
```
http {
  lightstep_access_token `your-access-token`;
  opentracing on;
...
```
Now, we'll see the following when admitting a new animal into the zoo:

![alt text](data/nginx-upload-trace1.png "Trace")

By default NGINX creates spans for both the request and the location blocks. It
uses the name of the first location as the name of the top-level span. We can
change this behavior by using the directives `opentracing_operation_name` and
`opentracing_location_operation_name` to change the names of the request and
location block spans respectively. We can also use the directive
`lightstep_component_name` to set a name to group together common traces. For
example, by adding 
```
http {
...
  lightstep_component_name zoo;
...
    location = /upload/animal {
      opentracing_location_operation_name upload;
    ...
```
The trace will change to

![alt text](data/nginx-upload-trace2.png "Trace")

At this point, similar information can be obtained by adding a logging
directive to the NGINX configuration. Advantages for OpenTracing are that the
data is easily accessible and searchable without needing to log onto a machine,
but much greater value can be realized if we also enable the backend for
OpenTracing.

Enabling OpenTracing for Backends
---------------------------------

When using express with Node.js, OpenTracing for requests can be easily enabled
by adding tracing middleware to the express app:
```JavaScript
const app = express();
app.use(tracingMiddleware.middleware({ tracer }));
...
```
When tracing is additionally manually added for the database and image
operations, we'll see the following when uploading:

![alt text](data/nginx-upload-trace3.png "Trace")


Performance Improvements
------------------------
