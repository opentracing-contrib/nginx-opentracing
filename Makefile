.PHONY: test
test:
	./ci/system_testing.sh

.PHONY: clean
clean:
	rm -fr test-log
