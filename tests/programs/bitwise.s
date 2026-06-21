section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 48
  mov     rax, 48
  mov     dword [rbp-4], eax
  mov     rax, 18
  mov     dword [rbp-8], eax
  mov     rax, 1
  push    rax
  mov     rax, 4
  mov     cl, al
  pop     rax
  shl     rax, cl
  mov     dword [rbp-12], eax
  mov     eax, dword [rbp-12]
  push    rax
  mov     rax, 31
  pop     r10
  and     rax, r10
  mov     dword [rbp-16], eax
  mov     eax, dword [rbp-16]
  push    rax
  mov     rax, 31
  pop     r10
  xor     rax, r10
  mov     dword [rbp-20], eax
  mov     eax, dword [rbp-12]
  push    rax
  mov     eax, dword [rbp-16]
  pop     r10
  or      rax, r10
  mov     dword [rbp-24], eax
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-8]
  pop     r10
  and     rax, r10
  mov     dword [rbp-28], eax
  mov     eax, dword [rbp-12]
  push    rax
  mov     eax, dword [rbp-16]
  pop     r10
  add     rax, r10
  push    rax
  mov     eax, dword [rbp-20]
  pop     r10
  add     rax, r10
  push    rax
  mov     eax, dword [rbp-24]
  pop     r10
  add     rax, r10
  push    rax
  mov     eax, dword [rbp-28]
  pop     r10
  add     rax, r10
  mov     dword [rbp-32], eax
  mov     rax, 100
  mov     dword [rbp-36], eax
  mov     rax, 7
  mov     dword [rbp-40], eax
  mov     eax, dword [rbp-36]
  push    rax
  mov     eax, dword [rbp-40]
  mov     r11, rax
  pop     rax
  cqo
  idiv    r11
  mov     dword [rbp-44], eax
  mov     eax, dword [rbp-36]
  push    rax
  mov     eax, dword [rbp-40]
  mov     r11, rax
  pop     rax
  cqo
  idiv    r11
  mov     rax, rdx
  mov     dword [rbp-48], eax
  mov     eax, dword [rbp-32]
  mov     eax, dword [rbp-32]
  push    rax
  mov     eax, dword [rbp-44]
  pop     r10
  add     rax, r10
  push    rax
  mov     eax, dword [rbp-48]
  pop     r10
  add     rax, r10
  mov     dword [rbp-32], eax
  mov     eax, dword [rbp-32]
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
