NGINX_VERSION=$(shell grep -m1 'FROM nginx:' <Dockerfile | awk -F'[: ]' '{print $$3}')

.PHONY: docker-image
docker-image:
	docker build -f Dockerfile -t opentracing-contrib/nginx-opentracing --target final .

.PHONY: docker-image-alpine
docker-image-alpine:
	docker build -f Dockerfile -t opentracing-contrib/nginx-opentracing --target final --build-arg BUILD_OS=alpine .

docker-build-binaries:
	docker buildx build --build-arg NGINX_VERSION=$(NGINX_VERSION) --platform linux/amd64 -f build/Dockerfile -t nginx-opentracing-binaries --target=export --output "type=local,dest=out" --progress=plain --no-cache --pull .

.PHONY: test
test:
	docker build -t nginx-opentracing-test/nginx -f test/Dockerfile-test . --build-arg NGINX_VERSION=$(NGINX_VERSION)
	docker build -t nginx-opentracing-test/backend -f test/Dockerfile-backend ./test
	docker build -t nginx-opentracing-test/grpc-backend -f test/environment/grpc/Dockerfile ./test/environment/grpc
	cd test && LOG_DIR=$(CURDIR)/test/test-log PYTHONPATH=environment/grpc python3 nginx_opentracing_test.py

.PHONY: clean
clean:
	rm -fr test/test-log
