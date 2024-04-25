.PHONY: docker-image
docker-image:
	DOCKER_BUILDKIT=1 docker build -f Dockerfile -t opentracing-contrib/nginx-opentracing --target final .

.PHONY: docker-image-alpine
docker-image-alpine:
	DOCKER_BUILDKIT=1 docker build -f Dockerfile -t opentracing-contrib/nginx-opentracing --target final --build-arg BUILD_OS=alpine .

docker-build-binaries:
	DOCKER_BUILDKIT=1 docker buildx build --build-arg NGINX_VERSION=1.26.0 --platform linux/amd64 -f build/Dockerfile -t nginx-opentracing-binaries --target=export --output "type=local,dest=out" --progress=plain --no-cache --pull .

.PHONY: test
test:
	./ci/system_testing.sh

.PHONY: clean
clean:
	rm -fr test-log
