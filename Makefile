.PHONY: docker-image
docker-image:
	DOCKER_BUILDKIT=1 docker build -f Dockerfile -t opentracing-contrib/nginx-opentracing --target final .

.PHONY: docker-image-alpine
docker-image-alpine:
	DOCKER_BUILDKIT=1 docker build -f Dockerfile -t opentracing-contrib/nginx-opentracing --target final --build-arg BUILD_OS=alpine .

.PHONY: test
test:
	./ci/system_testing.sh

.PHONY: clean
clean:
	rm -fr test-log
