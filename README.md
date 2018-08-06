# tracejump

Instrument assembly code with tracing of jump targets

To instrument a single assembly file `test.s` (e.g. for the empty program in `test.c`)

    python tracejump.py test.s test.instr.s

The `Makefile` contains some rules to produce instrumented assembly files `test.instr.s`, object files `test.instr.o`, and (single source) binaries `test.instr` from a source `test.c`.
For the latter, `__trace_jump.o` will be built and linked in automatically.

    make test.instr.s
    make test.instr.o
    make test.instr


Instrumentation algorithm taken from AFL `afl-as.c`.
See <http://lcamtuf.coredump.cx/afl/technical_details.txt> for further details.

Calls to `__trace_jump` are inserted textually

- after conditional jumps (not-taken branch)
- after labels

Assumes ATT syntax (as output by gcc).

Wishlist

- `as`, `cc`, `ld` wrappers (can instrument arbitrary builds easily as AFL does)
- make the tracing function configurable
