MODEL = model/nb_ptc.bin model/nb_classes.bin model/nb_pc.bin model/tk_nextmove.bin model/tk_output.bin

all: language_identifier $(MODEL)

model/%.bin: language_identifier.py model.py
	mkdir -p model
	python language_identifier.py split_models

language_identifier: language_identifier.c
	gcc -march=native -O2 -Wall -ansi -pedantic -std=c99 language_identifier.c -o language_identifier

check: language_identifier $(MODEL)
	bash -c "diff <(python language_identifier.py check) <(./language_identifier)"

clean:
	rm -rf language_identifier model

.PHONY: check clean
