# LightStep Example

A more complicated OpenTracing example demonstrating usage of the
nginx-opentracing docker image with lightstep. It features nginx serving as
a load balancer to multiple node servers. It provides a toy application for
a virtual zoo where animal profiles can be uploaded and viewed. Run

```bash
docker build -t nginx-example-zoo .
docker run -d -p 8080:80 -e LIGHTSTEP_ACCESS_TOKEN=YOURACCESSTOKEN nginx-example-zoo
```

and visit <http://localhost:8080> to interact with the application or your
LightStep account to view traces.
