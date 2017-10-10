A minimal OpenTracing example demonstrating usage of the nginx-opentracing docker 
image. It features Nginx as a reverse-proxy in front a Go server. Use these
commands to run:
```bash
docker build -t nginx-example-trivial .
docker run -d -p 9411:9411 --name zipkin openzipkin/zipkin
docker run -d -p 8080:80 --link zipkin:zipkin nginx-example-trivial
curl localhost:8080
```
Visit http://localhost:9411 to view the traces in Zipkin.
