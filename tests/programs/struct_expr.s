section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-8]
  mov     rax, 10
  mov     dword [rbp-8], eax
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-4]
  mov     rax, 20
  mov     dword [rbp-4], eax
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-8]
  push    rax
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-4]
  pop     r10
  add     rax, r10
  mov     dword [rbp-12], eax
  mov     eax, dword [rbp-12]
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
