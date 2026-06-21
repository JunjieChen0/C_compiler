section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     rax, 42
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-4]
  push    rax
  mov     rax, 8
  pop     r10
  add     rax, r10
  mov     dword [rbp-8], eax
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 2
  pop     r10
  imul    rax, r10
  mov     dword [rbp-12], eax
  mov     eax, dword [rbp-12]
  push    rax
  mov     eax, dword [rbp-4]
  pop     r10
  sub     r10, rax
  mov     rax, r10
  mov     dword [rbp-16], eax
  mov     eax, dword [rbp-16]
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
