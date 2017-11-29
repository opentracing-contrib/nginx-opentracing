An OpenTracing example demonstrating usage of the nginx-opentracing docker
image with jaeger. It builds on Jaeger's HotROD example (see blog post 
[Take OpenTracing for a HotROD ride](https://medium.com/opentracing/take-opentracing-for-a-hotrod-ride-f6e3141f7941)),
but inserts nginx as a load balancer for the HotROD services.

Use these commands to run:
```bash
docker build -t nginx-example-hotrod .
docker run -d -e COLLECTOR_ZIPKIN_HTTP_PORT=9411 -p5775:5775/udp -p6831:6831/udp -p6832:6832/udp \
  -p5778:5778 -p16686:16686 -p14268:14268 -p9411:9411 --name jaeger jaegertracing/all-in-one:latest
docker run -d -p 8080:80 --link jaeger:jaeger nginx-example-hotrod
```
Visit http://localhost:8080 to interact with the demo and http://localhost:16686
to view the traces in Jaeger.
