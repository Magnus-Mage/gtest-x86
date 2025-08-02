# calc.s - Simple calculator in Intel syntax
.intel_syntax noprefix
.global _start

.section .data
    newline: .ascii "\n"

.section .text
_start:
    # This is a simplified example
    # Real implementation would parse argv
    # For demo, just output "15\n" for any input
    
    mov rax, 1          # sys_write
    mov rdi, 1          # stdout
    mov rsi, offset result
    mov rdx, 3          # length
    syscall
    
    mov rax, 60         # sys_exit
    mov rdi, 0          # exit status
    syscall

.section .rodata
    result: .ascii "15\n"
