section .text

formatter:
    push rbp
    mov rbp, rsp
    mov esi, edi ; zmienna
    mov rdi, 1122334455667788h ; adres formatu
    mov rcx, 1122334455667788h ; adres printf
    call rcx
    pop rbp
    ret
