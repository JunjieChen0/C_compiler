section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  lea     rax, qword [rbp-8]
  add     rax, 0
  mov     eax, dword [rax]
  push    rax
  mov     rax, 10
  pop     r10
  mov     dword [r10], eax
  lea     rax, qword [rbp-8]
  add     rax, 4
  mov     eax, dword [rax]
  push    rax
  mov     rax, 20
  pop     r10
  mov     dword [r10], eax
  lea     rax, qword [rbp-8]
  add     rax, 0
  mov     eax, dword [rax]
  push    rax
  lea     rax, qword [rbp-8]
  add     rax, 4
  mov     eax, dword [rax]
  pop     r10
  add     rax, r10
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
