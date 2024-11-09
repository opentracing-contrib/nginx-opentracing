# OpenTelemetry Example

Demonstrating usage of the nginx-opentracing
docker image with Opentelemetry collector and Jaeger.
It features Nginx as a reverse-proxy in front a GO server.

Use these commands to run:

## Build nginx with opentracing and traces

From the root of the repo

```shell
make docker.build
```

## Run applications

From the current directory

```shell
docker compose up --build
```

Send requests to the app:

```shell
curl localhost:8081  # W3C format traces
curl localhost:8082  # Zipkin format traces
```

Visit <http://localhost:16686> to view the traces in Jaeger.
