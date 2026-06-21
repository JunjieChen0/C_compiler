section .text

global find_symbol
find_symbol:
  push    rbp
  mov     rbp, rsp
  mov     qword [rbp-8], rcx
  sub     rsp, 16
  mov     rax, 0
  mov     dword [rbp-12], eax
.L1:
  mov     eax, dword [rbp-12]
  push    rax
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  je      .L3
  push    rax
  mov     eax, dword [rbp-12]
  pop     r10
  imul    rax, 4
  add     rax, r10
  mov     eax, dword [rax]
  push    rax
  mov     rax, qword [rbp-8]
  push    rax
  pop     rdx
  pop     rcx
  call    strcmp
  push    rax
  mov     rax, 0
  pop     r10
  cmp     r10, rax
  setz    al
  movzx   rax, al
  cmp     rax, 0
  je      .L4
  push    rax
  mov     eax, dword [rbp-12]
  pop     r10
  imul    rax, 4
  add     rax, r10
  mov     eax, dword [rax]
  jmp     .L0
.L4:
.L2:
  mov     eax, dword [rbp-12]
  mov     r10, rax
  add     rax, 1
  mov     dword [rbp-12], eax
  mov     rax, r10
  jmp     .L1
.L3:
  mov     rax, 0
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret

global add_symbol
add_symbol:
  push    rbp
  mov     rbp, rsp
  mov     qword [rbp-24], r8
  mov     qword [rbp-16], rdx
  mov     qword [rbp-8], rcx
  sub     rsp, 32
  push    rax
  mov     rax, 100
  pop     r10
  cmp     r10, rax
  setge   al
  movzx   rax, al
  cmp     rax, 0
  je      .L7
  mov     rax, 0
  jmp     .L6
.L7:
  push    rax
  mov     r10, rax
  add     rax, 1
  mov     dword [rbp], eax
  mov     rax, r10
  pop     r10
  imul    rax, 4
  add     rax, r10
  mov     eax, dword [rax]
  mov     qword [rbp-32], rax
  mov     rax, qword [rbp-32]
  add     rax, 8
  mov     rax, qword [rax]
  mov     rax, qword [rbp-8]
  mov     rax, qword [rbp-32]
  add     rax, 0
  mov     eax, dword [rax]
  mov     eax, dword [rbp-16]
  mov     rax, qword [rbp-32]
  add     rax, 16
  mov     eax, dword [rax]
  mov     eax, dword [rbp-24]
  mov     rax, qword [rbp-32]
  jmp     .L6
.L6:
  mov     rsp, rbp
  pop     rbp
  ret

global string_length
string_length:
  push    rbp
  mov     rbp, rsp
  mov     qword [rbp-8], rcx
  sub     rsp, 16
  mov     rax, 0
  mov     dword [rbp-12], eax
.L10:
  mov     rax, qword [rbp-8]
  mov     r10, rax
  add     rax, 1
  mov     qword [rbp-8], rax
  mov     rax, r10
  mov     rax, byte [rax]
  cmp     rax, 0
  je      .L11
  mov     eax, dword [rbp-12]
  mov     r10, rax
  add     rax, 1
  mov     dword [rbp-12], eax
  mov     rax, r10
  jmp     .L10
.L11:
  mov     eax, dword [rbp-12]
  jmp     .L9
.L9:
  mov     rsp, rbp
  pop     rbp
  ret

global string_copy
string_copy:
  push    rbp
  mov     rbp, rsp
  mov     qword [rbp-16], rdx
  mov     qword [rbp-8], rcx
  sub     rsp, 32
  mov     rax, qword [rbp-8]
  mov     qword [rbp-24], rax
.L13:
  mov     rax, qword [rbp-16]
  mov     rax, byte [rax]
  cmp     rax, 0
  je      .L14
  mov     rax, qword [rbp-24]
  mov     r10, rax
  add     rax, 1
  mov     qword [rbp-24], rax
  mov     rax, r10
  mov     rax, byte [rax]
  push    rax
  mov     rax, qword [rbp-16]
  mov     r10, rax
  add     rax, 1
  mov     qword [rbp-16], rax
  mov     rax, r10
  mov     rax, byte [rax]
  pop     r10
  mov     byte [r10], al
  jmp     .L13
.L14:
  mov     rax, qword [rbp-24]
  mov     rax, byte [rax]
  push    rax
  mov     rax, 0
  pop     r10
  mov     byte [r10], al
  mov     rax, qword [rbp-8]
  jmp     .L12
.L12:
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
  mov     rax, 1
  push    rax
  mov     rax, 42
  push    rax
  pop     r8
  pop     rdx
  pop     rcx
  call    add_symbol
  mov     rax, .LC1
  push    rax
  mov     rax, 2
  push    rax
  mov     rax, 100
  push    rax
  pop     r8
  pop     rdx
  pop     rcx
  call    add_symbol
  mov     rax, .LC2
  push    rax
  pop     rcx
  call    find_symbol
  mov     qword [rbp-8], rax
  mov     rax, qword [rbp-8]
  cmp     rax, 0
  je      .L16
  mov     rax, qword [rbp-8]
  add     rax, 16
  mov     eax, dword [rax]
  jmp     .L15
.L16:
  mov     rax, 0
  jmp     .L15
.L15:
  mov     rsp, rbp
  pop     rbp
  ret

section .rodata
.LC0 db 120,0
.LC1 db 121,0
.LC2 db 120,0
