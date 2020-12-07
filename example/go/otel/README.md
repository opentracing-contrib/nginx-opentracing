Demonstrating usage of the nginx-opentracing
docker image with Opentelemetry collector and Jaeger. It features Nginx as a reverse-proxy in front a GO
server. Use these commands to run:

```bash
$ docker-compose up
$ curl localhost:8080
```

Visit http://localhost:16686 to view the traces in Jaeger.
