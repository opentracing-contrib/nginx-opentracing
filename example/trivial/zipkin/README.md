A minimal OpenTracing example demonstrating usage of the nginx-opentracing
docker image with Zipkin. It features Nginx as a reverse-proxy in front a Go
server. Use these commands to run:
```bash
docker-compose up
curl localhost:8080
```
Visit http://localhost:9411 to view the traces in Zipkin.
