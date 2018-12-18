.PHONY: all

%.s: %.c
	$(CC) -S -o $@ $^

%.instr.s: %.s
	./afl-as $^ $@

%.instr: %.instr.o __trace_jump.o
	$(CC) -o $@ $^

%.out.s: %
	objdump -d $^ > $@
