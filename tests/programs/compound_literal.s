section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     rax, 10
  mov     dword [rbp-16], eax
  mov     rax, 20
  mov     dword [rbp-12], eax
  mov     r10, qword [rbp-16]
  mov     qword [rbp-8], r10
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-8]
  push    rax
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-4]
  pop     r10
  add     rax, r10
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
