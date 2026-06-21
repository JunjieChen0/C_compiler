section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 32
  lea     rax, qword [rbp-16]
  mov     eax, dword [rbp-16]
  mov     rax, 10
  mov     dword [rbp-16], eax
  lea     rax, qword [rbp-16]
  mov     rax, qword [rbp-8]
  lea     rax, qword [rbp-32]
  lea     rax, qword [rbp-32]
  mov     qword [rbp-8], rax
  lea     rax, qword [rbp-32]
  mov     eax, dword [rbp-32]
  mov     rax, 20
  mov     dword [rbp-32], eax
  lea     rax, qword [rbp-32]
  mov     rax, qword [rbp-24]
  mov     rax, 0
  mov     qword [rbp-24], rax
  lea     rax, qword [rbp-16]
  mov     rax, qword [rbp-8]
  add     rax, 0
  mov     eax, dword [rax]
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
