section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  mov     rax, 42
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
