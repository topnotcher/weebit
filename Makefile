
.PHONY: test_progs
test_progs:
	make -C c
	make -C cpp

all: test_progs

test: test_progs
	python tests.py c
	python tests.py cpp

.PHONY: clean
clean:
	find . -name '*.o' -delete
	rm c/weebit
	rm cpp/weebit
