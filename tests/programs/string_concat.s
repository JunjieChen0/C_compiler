section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     rax, .LC0
  mov     qword [rbp-8], rax
  mov     rax, 0
  mov     dword [rbp-12], eax
.L1:
  mov     rax, qword [rbp-8]
  push    rax
  mov     eax, dword [rbp-12]
  pop     r10
  imul    rax, 4
  add     rax, r10
  mov     eax, dword [rax]
  cmp     rax, 0
  je      .L2
  mov     eax, dword [rbp-12]
  mov     r10, rax
  add     rax, 1
  mov     dword [rbp-12], eax
  mov     rax, r10
  jmp     .L1
.L2:
  mov     eax, dword [rbp-12]
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret

section .rodata
.LC0 db 72,101,108,108,111,32,87,111,114,108,100,0
