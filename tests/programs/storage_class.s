section .text

global helper
helper:
  push    rbp
  mov     rbp, rsp
  mov     qword [rbp-8], rcx
  sub     rsp, 16
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 2
  pop     r10
  imul    rax, r10
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     rax, 10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-4]
  push    rax
  pop     rcx
  call    helper
  push    rax
  pop     r10
  add     rax, r10
  mov     dword [rbp-8], eax
  mov     eax, dword [rbp-8]
  jmp     .L1
.L1:
  mov     rsp, rbp
  pop     rbp
  ret
