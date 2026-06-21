section .text

global factorial
factorial:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     rax, 1
  mov     dword [rbp-12], eax
  mov     rax, 1
  mov     dword [rbp-16], eax
.L1:
  mov     eax, dword [rbp-16]
  push    rax
  mov     eax, dword [rbp-8]
  pop     r10
  cmp     r10, rax
  setle   al
  movzx   rax, al
  cmp     rax, 0
  je      .L2
  mov     eax, dword [rbp-12]
  mov     eax, dword [rbp-12]
  push    rax
  mov     eax, dword [rbp-16]
  pop     r10
  imul    rax, r10
  mov     dword [rbp-12], eax
  mov     eax, dword [rbp-16]
  mov     eax, dword [rbp-16]
  push    rax
  mov     rax, 1
  pop     r10
  add     rax, r10
  mov     dword [rbp-16], eax
  jmp     .L1
.L2:
  mov     eax, dword [rbp-12]
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
  mov     rax, 5
  push    rax
  pop     rdi
  call    factorial
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-4]
  jmp     .L3
.L3:
  mov     rsp, rbp
  pop     rbp
  ret
