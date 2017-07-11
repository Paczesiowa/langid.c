ifndef RELEASE
	RELEASE = 1
endif

MODEL = model/nb_ptc.bin model/nb_classes.bin model/nb_pc.bin model/tk_nextmove.bin model/tk_output.bin

all: language_identifier_test $(MODEL)

model/%.bin: language_identifier.py model.py
	mkdir -p model
	python language_identifier.py split_models

language_identifier.o: language_identifier.c language_identifier.h
	gcc -c -march=native -O2 -Wall -ansi -pedantic -std=c99 language_identifier.c -o language_identifier.o -DRELEASE=${RELEASE}

language_identifier_test: language_identifier_test.c language_identifier.o
	gcc -march=native -O2 -Wall -ansi -pedantic -std=c99 language_identifier.o language_identifier_test.c -o language_identifier_test -latlcblas -lm

check: language_identifier_test $(MODEL)
	bash -c "diff <(python language_identifier.py check_model) <(./language_identifier_test check_model)"
	bash -c "diff <(python language_identifier.py check_output) <(./language_identifier_test check_output)"
	python language_identifier.py benchmark
	./language_identifier_test benchmark
	python language_identifier.py check_http_output
	python language_identifier.py check_zmq_output
	python language_identifier.py benchmark_http
	python language_identifier.py benchmark_zmq

clean:
	rm -rf language_identifier.o language_identifier_test model *.pyc

.PHONY: check clean
