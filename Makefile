.PHONY: docker.build
docker.build: test
	docker build -f Dockerfile -t opentracing-contrib/nginx-opentracing .

.PHONY: test
test:
	./ci/system_testing.sh

.PHONY: clean
clean:
	rm -fr test-log
