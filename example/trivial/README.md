A minimal OpenTracing example demonstrating usage of the nginx-opentracing
docker image with zipkin. It features Nginx as a reverse-proxy in front a Go
server. Use these commands to run:
```bash
docker build -t nginx-example-trivial .
docker run -d -p 9411:9411 --name zipkin openzipkin/zipkin
docker run -d -p 8080:80 --link zipkin:zipkin nginx-example-trivial
curl localhost:8080
```
Visit http://localhost:9411 to view the traces in Zipkin.

Additionaly, the example can be made to work with Jaeger. Run:
```bash
docker build -t nginx-example-trivial .
docker run -d -e COLLECTOR_ZIPKIN_HTTP_PORT=9411 -p5775:5775/udp -p6831:6831/udp -p6832:6832/udp -p5778:5778 -p16686:16686 -p14268:14268 -p9411:9411 --name jaeger jaegertracing/all-in-one:latest
docker run -d -p 8080:80 --link jaeger:zipkin nginx-example-trivial
curl localhost:8080
```
And you can view the traces at http://localhost:16686.
