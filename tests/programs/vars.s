section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     rax, 10
  mov     dword [rbp-4], eax
  mov     rax, 20
  mov     dword [rbp-8], eax
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-8]
  pop     r10
  add     rax, r10
  mov     dword [rbp-12], eax
  mov     eax, dword [rbp-12]
  push    rax
  mov     rax, 30
  pop     r10
  cmp     r10, rax
  setz    al
  movzx   rax, al
  cmp     rax, 0
  je      .L1
  mov     rax, 0
  jmp     .L0
.L1:
  mov     rax, 1
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
