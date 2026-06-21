section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     rax, 0
  mov     dword [rbp-4], eax
  mov     rax, 0
  mov     dword [rbp-8], eax
.L1:
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 10
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  je      .L2
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-8]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-8]
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 1
  pop     r10
  add     rax, r10
  mov     dword [rbp-8], eax
  jmp     .L1
.L2:
  mov     eax, dword [rbp-4]
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
