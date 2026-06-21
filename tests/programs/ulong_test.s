section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     rax, 100
  mov     qword [rbp-8], rax
  mov     rax, qword [rbp-8]
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
