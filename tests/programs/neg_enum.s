section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     rax, -1
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-4]
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
