section .text

global sum
sum:
  push    rbp
  mov     rbp, rsp
  mov     qword [rbp-8], rcx
  sub     rsp, 32
  mov     rax, qword [rbp-16]
  mov     eax, dword [rbp-8]
  lea     rax, qword [rbp-8]
  push    rax
  mov     rax, 8
  pop     r10
  sub     r10, rax
  mov     rax, r10
  mov     qword [rbp-16], rax
  mov     rax, 0
  mov     dword [rbp-20], eax
  mov     rax, 0
  mov     dword [rbp-24], eax
.L1:
  mov     eax, dword [rbp-24]
  push    rax
  mov     eax, dword [rbp-8]
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  je      .L3
  mov     eax, dword [rbp-20]
  push    rax
  mov     rax, qword [rbp-16]
  push    rax
  mov     rax, 8
  pop     r10
  sub     r10, rax
  mov     rax, r10
  mov     qword [rbp-16], rax
  mov     rax, qword [rbp-16]
  mov     rax, dword [rax]
  pop     r10
  add     r10, rax
  mov     rax, r10
  mov     dword [rbp-20], eax
.L2:
  mov     eax, dword [rbp-24]
  mov     r10, rax
  add     rax, 1
  mov     dword [rbp-24], eax
  mov     rax, r10
  jmp     .L1
.L3:
  mov     rax, qword [rbp-16]
  mov     rax, 0
  mov     qword [rbp-16], rax
  mov     eax, dword [rbp-20]
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
  mov     rax, 3
  push    rax
  mov     rax, 10
  push    rax
  mov     rax, 20
  push    rax
  mov     rax, 30
  push    rax
  pop     r9
  pop     r8
  pop     rdx
  pop     rcx
  call    sum
  mov     dword [rbp-4], eax
  mov     rax, 2
  push    rax
  mov     rax, 100
  push    rax
  mov     rax, 200
  push    rax
  pop     r8
  pop     rdx
  pop     rcx
  call    sum
  mov     dword [rbp-8], eax
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-8]
  pop     r10
  add     rax, r10
  jmp     .L4
.L4:
  mov     rsp, rbp
  pop     rbp
  ret
