A port of the [zipkin-js-example/web example](https://github.com/openzipkin/zipkin-js-example/tree/master/web) to use nginx.
It features Nginx as a reverse-proxy in front node services and client-side tracing in the browser.
Use these commands to run:
```bash
docker build -t nginx-example-browser .
docker run -d -p 9411:9411 --name zipkin openzipkin/zipkin
docker run -d -p 8080:80 --link zipkin:zipkin nginx-example-browser
```
Visit http://localhost:8080 in your browser and http://localhost:9411 to view the traces in Zipkin.
