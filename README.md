# tracejump

Instrument assembly code with tracing of jump targets

Usage

    python tracejump.py <in> <out>

Instrumentation algorithm taken from AFL `afl-as.c`.
See <http://lcamtuf.coredump.cx/afl/technical_details.txt> for further details.

Calls to `__trace_jump` are inserted textually

- after conditional jumps (not-taken branch)
- after labels

Assumes ATT syntax (as output by gcc).

Wishlist

- `as`, `cc`, `ld` wrappers (can instrument arbitrary builds easily as AFL does)
- make the tracing function configurable
