#!/usr/bin/env python

# See AFL: afl-as.c
# http://lcamtuf.coredump.cx/afl/technical_details.txt

import sys

code_sections = [
    '\t.text\n',
    '\t.section\t.text',
    '\t.section\t__TEXT,__text',
    '\t.section __TEXT,__text'
    ]

data_sections = [
    '\t.section',
    '\t.',
    '\t.',
    '\t.'
    ]

is64 = True


def trace_jump(output):
    global is64
    if(is64):
        output.write('\tcall\t__trace_jump\n')
    else:
        output.write('\tcall\t__trace_jump\n')


# see afl-as.c add_instrumentation
def instrument(input, output):
    global code_sections, data_sections, is64
    
    ins_lines = 0
    instr_ok = False
    skip_csect = False
    skip_next_label = False
    skip_intel = False
    skip_app = False
    instrument_next = False
    
    for line in input:
        if not skip_intel and not skip_app and not skip_csect and instr_ok and instrument_next and line[0] == '\t' and str.isalpha(line[1]):
            trace_jump(output)
            ins_lines += 1
            instrument_next = False

        output.write(line)
        
        if(line[0] == '\t' and line[1] == '.'):
            if line == '\t.text\n' or line.startswith('\t.section\t.text') or line.startswith('\t.section\t__TEXT,__text') or line.startswith('\t.section __TEXT,__text'):
                instr_ok = True
                continue
            
            if line == '\t.bss\n' or line == '\t.data\n' or line.startswith('\t.section\t') or line.startswith('\t.section '):
                instr_ok = False
                continue
            
            if '.code' in line:
                if '.code32' in line:
                    is64 = False
                if '.code64' in line:
                    is64 = True
                    
            if '.intel_syntax' in line:
                skip_intel = True
            if '.att_syntax' in line:
                skip_intel = False
                
        if line.startswith('##'):
            if '#APP' in line:
                skip_app = True
            if '#NO_APP' in line:
                skip_app = False
                
        if skip_intel or skip_app or skip_csect or not instr_ok or line[0] == '#' or line[0] == ' ':
            continue
        
        if line[0] == '\t':
            if line[1] == 'j' and line[2] != 'm':
                trace_jump(output)
                ins_lines += 1
            continue
        
        if line[0] == '.' and ':' in line:
            if str.isdigit(line[2]):
                if not skip_next_label:
                    instrument_next = True
                else:
                    skip_next_label = False
            else:
                instrument_next = True
                    
    return ins_lines


if __name__ == "__main__" and len(sys.argv) > 2:
    input = open(sys.argv[1], 'r')
    output = open(sys.argv[2], 'w')
    ins_lines = instrument(input, output)
    print('instrumented ' + str(ins_lines) + ' lines')
