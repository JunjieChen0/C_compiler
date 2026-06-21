section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 32
  mov     rax, 10
  mov     dword [rbp-4], eax
  mov     rax, 20
  mov     dword [rbp-8], eax
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-8]
  pop     r10
  add     rax, r10
  mov     dword [rbp-12], eax
  mov     rax, 100
  mov     dword [rbp-16], eax
  mov     rax, 200
  mov     dword [rbp-20], eax
  mov     eax, dword [rbp-16]
  push    rax
  mov     eax, dword [rbp-20]
  pop     r10
  imul    rax, r10
  mov     dword [rbp-24], eax
  mov     eax, dword [rbp-12]
  push    rax
  mov     eax, dword [rbp-24]
  pop     r10
  add     rax, r10
  mov     dword [rbp-28], eax
  mov     eax, dword [rbp-28]
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
