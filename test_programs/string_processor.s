# string_processor.s - Convert input to uppercase
.intel_syntax noprefix
.global _start

.section .bss
    buffer: .space 1024

.section .text
_start:
    # Read from stdin
    mov rax, 0          # sys_read
    mov rdi, 0          # stdin
    mov rsi, offset buffer
    mov rdx, 1024       # max bytes
    syscall
    
    mov r8, rax         # save read count
    
    # Convert to uppercase (simplified)
    mov rdi, offset buffer
    mov rcx, r8
    
convert_loop:
    cmp rcx, 0
    je write_output
    
    mov al, [rdi]
    cmp al, 'a'
    jl next_char
    cmp al, 'z'
    jg next_char
    
    sub al, 32          # Convert to uppercase
    mov [rdi], al
    
next_char:
    inc rdi
    dec rcx
    jmp convert_loop
    
write_output:
    mov rax, 1          # sys_write
    mov rdi, 1          # stdout
    mov rsi, offset buffer
    mov rdx, r8         # bytes to write
    syscall
    
    mov rax, 60         # sys_exit
    mov rdi, 0
    syscall
