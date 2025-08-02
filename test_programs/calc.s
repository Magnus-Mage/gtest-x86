# calc.s - Functional calculator in Intel syntax
.intel_syntax noprefix
.global _start

.section .text
_start:
    # Get argc from stack
    mov rdi, [rsp]      # argc
    cmp rdi, 4          # Need at least 4 args: program, num1, num2, operation
    jl error_exit
    
    # Get argv pointers
    lea rsi, [rsp + 8]  # argv array
    
    # Parse first number (argv[1])
    mov rdi, [rsi + 8]  # argv[1]
    call atoi
    mov r12, rax        # Store first number in r12
    
    # Parse second number (argv[2])
    mov rdi, [rsi + 16] # argv[2]
    call atoi
    mov r13, rax        # Store second number in r13
    
    # Get operation string (argv[3])
    mov rdi, [rsi + 24] # argv[3]
    
    # Check operation type
    mov al, [rdi]       # First character of operation
    cmp al, 'a'         # add
    je do_add
    cmp al, 's'         # sub
    je do_sub  
    cmp al, 'm'         # mul
    je do_mul
    cmp al, 'd'         # div
    je do_div
    jmp error_exit
    
do_add:
    mov rax, r12
    add rax, r13
    jmp print_result
    
do_sub:
    mov rax, r12
    sub rax, r13
    jmp print_result
    
do_mul:
    mov rax, r12
    imul rax, r13
    jmp print_result
    
do_div:
    cmp r13, 0
    je div_by_zero
    mov rax, r12
    cqo                 # Sign extend rax to rdx:rax
    idiv r13
    jmp print_result

div_by_zero:
    mov rax, 1          # sys_write
    mov rdi, 2          # stderr
    mov rsi, offset div_error_msg
    mov rdx, div_error_len
    syscall
    mov rdi, 1          # Exit with error code 1
    jmp exit_program

print_result:
    # Convert result to string and print
    mov rdi, rax
    call print_number
    
    # Print newline
    mov rax, 1          # sys_write
    mov rdi, 1          # stdout
    mov rsi, offset newline
    mov rdx, 1
    syscall
    
    mov rdi, 0          # Success exit code
    jmp exit_program

error_exit:
    mov rax, 1          # sys_write
    mov rdi, 2          # stderr
    mov rsi, offset usage_msg
    mov rdx, usage_len
    syscall
    mov rdi, 1          # Error exit code

exit_program:
    mov rax, 60         # sys_exit
    syscall

# Simple atoi implementation
atoi:
    xor rax, rax        # Result = 0
    xor rdx, rdx        # Sign = positive
    mov rsi, rdi        # String pointer
    
    # Check for negative sign
    cmp byte ptr [rsi], '-'
    jne atoi_loop
    inc rdx             # Set sign flag
    inc rsi             # Skip minus sign
    
atoi_loop:
    mov cl, [rsi]       # Get character
    test cl, cl         # Check for null terminator
    jz atoi_done
    
    cmp cl, '0'
    jl atoi_done
    cmp cl, '9'
    jg atoi_done
    
    sub cl, '0'         # Convert to digit
    imul rax, 10        # Result *= 10
    add rax, rcx        # Result += digit
    inc rsi
    jmp atoi_loop
    
atoi_done:
    test rdx, rdx       # Check sign
    jz atoi_return
    neg rax             # Make negative
atoi_return:
    ret

# Print number implementation
print_number:
    # Handle negative numbers
    test rdi, rdi
    jns print_positive
    
    # Print minus sign
    push rdi
    mov rax, 1
    mov rdi, 1
    mov rsi, offset minus_sign
    mov rdx, 1
    syscall
    pop rdi
    neg rdi
    
print_positive:
    # Convert to string (reverse order)
    mov rax, rdi
    mov rcx, 0          # Digit counter
    lea rsi, [number_buffer + 19]  # Start at end of buffer
    mov byte ptr [rsi + 1], 0      # Null terminator
    
convert_loop:
    xor rdx, rdx
    mov rbx, 10
    div rbx             # rax = rax/10, rdx = rax%10
    add dl, '0'         # Convert remainder to ASCII
    mov [rsi], dl
    dec rsi
    inc rcx
    test rax, rax
    jnz convert_loop
    
    # Print the string
    inc rsi             # Point to first digit
    mov rax, 1          # sys_write
    mov rdi, 1          # stdout
    mov rdx, rcx        # Length
    syscall
    ret

.section .data
    newline: .ascii "\n"
    minus_sign: .ascii "-"
    div_error_msg: .ascii "Error: division by zero\n"
    div_error_len = . - div_error_msg
    usage_msg: .ascii "Usage: calc <num1> <num2> <operation>\nOperations: add, sub, mul, div\n"
    usage_len = . - usage_msg

.section .bss
    number_buffer: .space 21    # Buffer for number string conversion
