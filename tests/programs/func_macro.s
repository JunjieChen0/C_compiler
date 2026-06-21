section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     rax, 5
  mov     dword [rbp-4], eax
  mov     rax, 10
  mov     dword [rbp-8], eax
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-8]
  pop     r10
  cmp     r10, rax
  setg    al
  movzx   rax, al
  cmp     rax, 0
  je      .L1
  mov     eax, dword [rbp-4]
  jmp     .L2
.L1:
  mov     eax, dword [rbp-8]
.L2:
  mov     dword [rbp-12], eax
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-4]
  pop     r10
  imul    rax, r10
  mov     dword [rbp-16], eax
  mov     eax, dword [rbp-12]
  push    rax
  mov     eax, dword [rbp-16]
  pop     r10
  add     rax, r10
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
