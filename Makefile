.PHONY: all

all: test.instr

%.s: %.c
	$(CC) -S -o $@ $^

%.instr.s: %.s
	python tracejump.py $^ $@

%.instr: %.instr.o __trace_jump.o
	$(CC) -o $@ $^

%.out.s: %
	objdump -d $^ > $@
