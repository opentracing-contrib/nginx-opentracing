.PHONY: docker.build
docker.build: test
	docker build --build-arg NGINX_LABEL=1.19.5 -f Dockerfile -t opentracing-contrib/nginx-opentracing .
