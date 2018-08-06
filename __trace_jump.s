.global __trace_jump

__trace_jump:
    push   %rbp
    mov    %rsp,%rbp
    mov    0x8(%rbp),%rax   # get return address
    sub    $0x5,%rax        # subtract 5 to move before the call
    mov    %rax,-0x8(%rbp)  # store on stack

	mov    $0x1,%rax        # system call number 1 = write
    mov    $0x1,%rdi        # file handle        1 = stdout
    mov    %rbp,%rsi        # 
    sub    $0x8,%rsi
    mov    $0x8,%rdx
    syscall

    pop    %rbp
    ret
