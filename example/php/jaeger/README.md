Demonstrating usage of the nginx-opentracing
docker image with Jaeger and FastCGI. It features Nginx as a reverse-proxy in front a PHP
server. Use these commands to run:
```bash
docker-compose up
curl localhost:8080
```
Visit http://localhost:16686 to view the traces in Jaeger.
