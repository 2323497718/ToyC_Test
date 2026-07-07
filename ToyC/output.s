.intel_syntax noprefix
.text

.globl main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    push rbx
    push r12
    push r13
    push r14
    push r15

entry0:
    mov DWORD PTR [rbp-8], 10
    mov eax, [rbp-8]
    mov [rbp-12], eax
    mov eax, [rbp-12]
    mov ecx, 5
    cmp eax, ecx
    setg al
    movzx eax, al
    mov [rbp-16], eax
    cmp eax, 0
    je else1
    mov DWORD PTR [rbp-8], 1
    jmp endif2
else1:
    mov DWORD PTR [rbp-8], 0
endif2:
    mov eax, [rbp-8]
    mov [rbp-20], eax
    jmp main.epilog

main.epilog:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    leave
    ret
