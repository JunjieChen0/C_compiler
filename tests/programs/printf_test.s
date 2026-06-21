section .text

global my_printf
my_printf:
  push    rbp
  mov     rbp, rsp
  mov     qword [rbp-8], rcx
  sub     rsp, 64
  mov     rax, qword [rbp-16]
  mov     rax, qword [rbp-8]
  lea     rax, qword [rbp-8]
  push    rax
  mov     rax, 8
  pop     r10
  sub     r10, rax
  mov     rax, r10
  mov     qword [rbp-16], rax
  mov     rax, 0
  mov     dword [rbp-20], eax
  mov     rax, qword [rbp-8]
  mov     qword [rbp-32], rax
.L1:
  mov     rax, qword [rbp-32]
  mov     rax, byte [rax]
  cmp     rax, 0
  je      .L2
  mov     rax, qword [rbp-32]
  mov     rax, byte [rax]
  push    rax
  mov     rax, 37
  pop     r10
  cmp     r10, rax
  setz    al
  movzx   rax, al
  cmp     rax, 0
  je      .L3
  mov     rax, qword [rbp-32]
  mov     r10, rax
  add     rax, 1
  mov     qword [rbp-32], rax
  mov     rax, r10
  mov     rax, qword [rbp-32]
  mov     rax, byte [rax]
  push    rax
  mov     rax, 100
  pop     r10
  cmp     r10, rax
  setz    al
  movzx   rax, al
  cmp     rax, 0
  je      .L5
  mov     rax, qword [rbp-16]
  push    rax
  mov     rax, 8
  pop     r10
  sub     r10, rax
  mov     rax, r10
  mov     qword [rbp-16], rax
  mov     rax, qword [rbp-16]
  mov     rax, dword [rax]
  mov     dword [rbp-36], eax
  mov     eax, dword [rbp-36]
  push    rax
  mov     rax, 0
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  je      .L7
  mov     eax, dword [rbp-20]
  mov     r10, rax
  add     rax, 1
  mov     dword [rbp-20], eax
  mov     rax, r10
  mov     eax, dword [rbp-36]
  mov     eax, dword [rbp-36]
  neg     rax
  mov     dword [rbp-36], eax
.L7:
  mov     rax, 0
  mov     dword [rbp-40], eax
  mov     eax, dword [rbp-36]
  mov     dword [rbp-44], eax
.L9:
  mov     eax, dword [rbp-40]
  mov     r10, rax
  add     rax, 1
  mov     dword [rbp-40], eax
  mov     rax, r10
  mov     eax, dword [rbp-44]
  push    rax
  mov     rax, 10
  pop     r10
  mov     rax, r10
  mov     dword [rbp-44], eax
.L10:
  mov     eax, dword [rbp-44]
  push    rax
  mov     rax, 0
  pop     r10
  cmp     r10, rax
  setg    al
  movzx   rax, al
  cmp     rax, 0
  jne     .L9
.L11:
  mov     eax, dword [rbp-20]
  push    rax
  mov     eax, dword [rbp-40]
  pop     r10
  add     r10, rax
  mov     rax, r10
  mov     dword [rbp-20], eax
  jmp     .L6
.L5:
  mov     rax, qword [rbp-32]
  mov     rax, byte [rax]
  push    rax
  mov     rax, 115
  pop     r10
  cmp     r10, rax
  setz    al
  movzx   rax, al
  cmp     rax, 0
  je      .L12
  mov     rax, qword [rbp-16]
  push    rax
  mov     rax, 8
  pop     r10
  sub     r10, rax
  mov     rax, r10
  mov     qword [rbp-16], rax
  mov     rax, qword [rbp-16]
  mov     rax, qword [rax]
  mov     qword [rbp-56], rax
.L14:
  mov     rax, qword [rbp-56]
  mov     rax, byte [rax]
  cmp     rax, 0
  je      .L15
  mov     eax, dword [rbp-20]
  mov     r10, rax
  add     rax, 1
  mov     dword [rbp-20], eax
  mov     rax, r10
  mov     rax, qword [rbp-56]
  mov     r10, rax
  add     rax, 1
  mov     qword [rbp-56], rax
  mov     rax, r10
  jmp     .L14
.L15:
  jmp     .L13
.L12:
  mov     rax, qword [rbp-32]
  mov     rax, byte [rax]
  push    rax
  mov     rax, 37
  pop     r10
  cmp     r10, rax
  setz    al
  movzx   rax, al
  cmp     rax, 0
  je      .L16
  mov     eax, dword [rbp-20]
  mov     r10, rax
  add     rax, 1
  mov     dword [rbp-20], eax
  mov     rax, r10
.L16:
.L13:
.L6:
  jmp     .L4
.L3:
  mov     eax, dword [rbp-20]
  mov     r10, rax
  add     rax, 1
  mov     dword [rbp-20], eax
  mov     rax, r10
.L4:
  mov     rax, qword [rbp-32]
  mov     r10, rax
  add     rax, 1
  mov     qword [rbp-32], rax
  mov     rax, r10
  jmp     .L1
.L2:
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
  mov     rax, .LC0
  push    rax
  mov     rax, .LC1
  push    rax
  mov     rax, 42
  push    rax
  pop     r8
  pop     rdx
  pop     rcx
  call    my_printf
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-4]
  jmp     .L18
.L18:
  mov     rsp, rbp
  pop     rbp
  ret

section .rodata
.LC0 db 72,101,108,108,111,32,37,115,44,32,121,111,117,32,104,97,118,101,32,37,100,32,109,101,115,115,97,103,101,115,0
.LC1 db 87,111,114,108,100,0
