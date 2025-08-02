# calc.s - Working calculator in Intel syntax
.intel_syntax noprefix
.global _start

.section .text
_start:
    # Get argc from stack
    mov rdi, [rsp]      # argc is at [rsp]
    cmp rdi, 4          # Need 4 args: program, num1, num2, operation
    jl usage_error
    
    # Get argv[1] - first number
    mov rdi, [rsp + 16] # argv[1] 
    call atoi
    mov r12, rax        # Save first number
    
    # Get argv[2] - second number  
    mov rdi, [rsp + 24] # argv[2]
    call atoi
    mov r13, rax        # Save second number
    
    # Get argv[3] - operation
    mov rdi, [rsp + 32] # argv[3]
    mov al, [rdi]       # First character
    
    cmp al, 'a'         # add
    je do_add
    cmp al, 's'         # sub
    je do_sub
    cmp al, 'm'         # mul
    je do_mul  
    cmp al, 'd'         # div
    je do_div
    jmp usage_error

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
    je div_error
    mov rax, r12
    cqo                 # Sign extend
    idiv r13
    jmp print_result

div_error:
    mov rax, 1          # sys_write
    mov rdi, 2          # stderr
    lea rsi, [rip + div_msg]
    mov rdx, div_msg_len
    syscall
    mov rdi, 1
    jmp exit_program

print_result:
    mov rdi, rax
    call print_int
    
    # Print newline
    mov rax, 1          # sys_write
    mov rdi, 1          # stdout
    lea rsi, [rip + newline]
    mov rdx, 1
    syscall
    
    mov rdi, 0          # Success
    jmp exit_program

usage_error:
    mov rax, 1          # sys_write
    mov rdi, 2          # stderr
    lea rsi, [rip + usage_msg]
    mov rdx, usage_msg_len
    syscall
    mov rdi, 1          # Error code

exit_program:
    mov rax, 60         # sys_exit
    syscall

# Convert string to integer
# Input: rdi = string pointer
# Output: rax = integer
atoi:
    xor rax, rax        # result = 0
    xor rcx, rcx        # sign = 0
    mov rsi, rdi        # string pointer
    
    # Check for minus sign
    cmp byte ptr [rsi], '-'
    jne atoi_loop
    mov rcx, 1          # negative flag
    inc rsi             # skip minus
    
atoi_loop:
    mov dl, [rsi]       # get character
    test dl, dl         # check for null
    jz atoi_done
    
    cmp dl, '0'
    jl atoi_done
    cmp dl, '9'
    jg atoi_done
    
    sub dl, '0'         # convert to digit
    imul rax, 10        # result *= 10
    movzx rdx, dl       # zero extend
    add rax, rdx        # result += digit
    inc rsi
    jmp atoi_loop

atoi_done:
    test rcx, rcx       # check sign
    jz atoi_return
    neg rax             # make negative
atoi_return:
    ret

# Print integer
# Input: rdi = integer to print
print_int:
    test rdi, rdi
    jns print_positive
    
    # Print minus sign
    push rdi
    mov rax, 1
    mov rdi, 1
    lea rsi, [rip + minus_sign]
    mov rdx, 1
    syscall
    pop rdi
    neg rdi

print_positive:
    mov rax, rdi
    lea rsi, [rip + number_buffer + 19]  # End of buffer
    mov byte ptr [rsi + 1], 0            # Null terminator
    mov rcx, 0                           # Digit count

convert_loop:
    xor rdx, rdx
    mov rbx, 10
    div rbx             # rax = rax/10, rdx = remainder
    add dl, '0'         # Convert to ASCII
    mov [rsi], dl
    dec rsi
    inc rcx
    test rax, rax
    jnz convert_loop
    
    # Print the number
    inc rsi             # Point to first digit
    mov rax, 1          # sys_write
    mov rdi, 1          # stdout
    mov rdx, rcx        # length
    syscall
    ret

.section .data
newline:        .ascii "\n"
minus_sign:     .ascii "-"  
div_msg:        .ascii "Error: division by zero\n"
div_msg_len = . - div_msg
usage_msg:      .ascii "Usage: calc <num1> <num2> <operation>\nOperations: add, sub, mul, div\n"
usage_msg_len = . - usage_msg

.section .bss
number_buffer:  .space 21
