section .text

global fibonacci
fibonacci:
  push    rbp
  mov     rbp, rsp
  mov     qword [rbp-8], rcx
  sub     rsp, 16
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 1
  pop     r10
  cmp     r10, rax
  setle   al
  movzx   rax, al
  cmp     rax, 0
  je      .L1
  mov     eax, dword [rbp-8]
  jmp     .L0
.L1:
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 1
  pop     r10
  sub     r10, rax
  mov     rax, r10
  push    rax
  pop     rcx
  call    fibonacci
  push    rax
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 2
  pop     r10
  sub     r10, rax
  mov     rax, r10
  push    rax
  pop     rcx
  call    fibonacci
  pop     r10
  add     rax, r10
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret

global factorial
factorial:
  push    rbp
  mov     rbp, rsp
  mov     qword [rbp-8], rcx
  sub     rsp, 16
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 1
  pop     r10
  cmp     r10, rax
  setle   al
  movzx   rax, al
  cmp     rax, 0
  je      .L4
  mov     rax, 1
  jmp     .L3
.L4:
  mov     eax, dword [rbp-8]
  push    rax
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 1
  pop     r10
  sub     r10, rax
  mov     rax, r10
  push    rax
  pop     rcx
  call    factorial
  pop     r10
  imul    rax, r10
  jmp     .L3
.L3:
  mov     rsp, rbp
  pop     rbp
  ret

global gcd
gcd:
  push    rbp
  mov     rbp, rsp
  mov     qword [rbp-16], rdx
  mov     qword [rbp-8], rcx
  sub     rsp, 32
.L7:
  mov     eax, dword [rbp-16]
  push    rax
  mov     rax, 0
  pop     r10
  cmp     r10, rax
  setnz   al
  movzx   rax, al
  cmp     rax, 0
  je      .L8
  mov     eax, dword [rbp-16]
  mov     dword [rbp-20], eax
  mov     eax, dword [rbp-16]
  mov     eax, dword [rbp-8]
  push    rax
  mov     eax, dword [rbp-16]
  mov     r11, rax
  pop     rax
  cqo
  idiv    r11
  mov     rax, rdx
  mov     dword [rbp-16], eax
  mov     eax, dword [rbp-8]
  mov     eax, dword [rbp-20]
  mov     dword [rbp-8], eax
  jmp     .L7
.L8:
  mov     eax, dword [rbp-8]
  jmp     .L6
.L6:
  mov     rsp, rbp
  pop     rbp
  ret

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     rax, 10
  push    rax
  pop     rcx
  call    fibonacci
  mov     dword [rbp-4], eax
  mov     rax, 5
  push    rax
  pop     rcx
  call    factorial
  mov     dword [rbp-8], eax
  mov     rax, 48
  push    rax
  mov     rax, 18
  push    rax
  pop     rdx
  pop     rcx
  call    gcd
  mov     dword [rbp-12], eax
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-8]
  pop     r10
  add     rax, r10
  push    rax
  mov     eax, dword [rbp-12]
  pop     r10
  add     rax, r10
  jmp     .L9
.L9:
  mov     rsp, rbp
  pop     rbp
  ret
