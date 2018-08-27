A minimal OpenTracing example demonstrating usage of the nginx-opentracing
docker image with Datadog. It features Nginx as a reverse-proxy in front a Go
server. 

Before running create an account at https://www.datadoghq.com and enter your
API key into docker-compose.yaml

Use these commands to run:
```bash
docker-compose up
curl localhost:8080
```
Visit https://app.datadoghq.com/apm/services to view the traces.
