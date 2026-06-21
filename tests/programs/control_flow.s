section .text

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 32
  mov     rax, 0
  mov     dword [rbp-4], eax
  mov     rax, 0
  mov     dword [rbp-8], eax
.L1:
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 10
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  je      .L2
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-8]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-8]
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 1
  pop     r10
  add     rax, r10
  mov     dword [rbp-8], eax
  jmp     .L1
.L2:
  mov     rax, 0
  mov     dword [rbp-12], eax
.L3:
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-12]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-12]
  mov     eax, dword [rbp-12]
  push    rax
  mov     rax, 1
  pop     r10
  add     rax, r10
  mov     dword [rbp-12], eax
.L4:
  mov     eax, dword [rbp-12]
  push    rax
  mov     rax, 5
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  jne     .L3
.L5:
  mov     eax, dword [rbp-16]
  mov     rax, 0
  mov     dword [rbp-16], eax
.L6:
  mov     eax, dword [rbp-16]
  push    rax
  mov     rax, 10
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  je      .L8
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-16]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
.L7:
  mov     eax, dword [rbp-16]
  mov     r10, rax
  add     rax, 1
  mov     dword [rbp-16], eax
  mov     rax, r10
  jmp     .L6
.L8:
  mov     eax, dword [rbp-4]
  push    rax
  mov     rax, 100
  pop     r10
  cmp     r10, rax
  setg    al
  movzx   rax, al
  cmp     rax, 0
  je      .L9
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     rax, 50
  pop     r10
  sub     r10, rax
  mov     rax, r10
  mov     dword [rbp-4], eax
  jmp     .L10
.L9:
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     rax, 50
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
.L10:
  mov     rax, 42
  mov     dword [rbp-20], eax
  mov     eax, dword [rbp-20]
  push    rax
  mov     rax, 42
  pop     r10
  cmp     r10, rax
  setz    al
  movzx   rax, al
  cmp     rax, 0
  je      .L11
  mov     eax, dword [rbp-20]
  mov     rax, 1
  mov     dword [rbp-20], eax
  jmp     .L12
.L11:
  mov     eax, dword [rbp-20]
  push    rax
  mov     rax, 43
  pop     r10
  cmp     r10, rax
  setz    al
  movzx   rax, al
  cmp     rax, 0
  je      .L13
  mov     eax, dword [rbp-20]
  mov     rax, 2
  mov     dword [rbp-20], eax
  jmp     .L14
.L13:
  mov     eax, dword [rbp-20]
  mov     rax, 3
  mov     dword [rbp-20], eax
.L14:
.L12:
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-20]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     rax, 0
  mov     dword [rbp-24], eax
.L15:
  mov     eax, dword [rbp-24]
  push    rax
  mov     rax, 20
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  je      .L16
  mov     eax, dword [rbp-24]
  mov     eax, dword [rbp-24]
  push    rax
  mov     rax, 1
  pop     r10
  add     rax, r10
  mov     dword [rbp-24], eax
  mov     eax, dword [rbp-24]
  push    rax
  mov     rax, 2
  mov     r11, rax
  pop     rax
  cqo
  idiv    r11
  mov     rax, rdx
  push    rax
  mov     rax, 0
  pop     r10
  cmp     r10, rax
  setz    al
  movzx   rax, al
  cmp     rax, 0
  je      .L17
  jmp     .L15
.L17:
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-24]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  jmp     .L15
.L16:
  mov     rax, 0
  mov     dword [rbp-28], eax
.L19:
  mov     eax, dword [rbp-28]
  push    rax
  mov     rax, 100
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  je      .L20
  mov     eax, dword [rbp-28]
  mov     eax, dword [rbp-28]
  push    rax
  mov     rax, 1
  pop     r10
  add     rax, r10
  mov     dword [rbp-28], eax
  mov     eax, dword [rbp-28]
  push    rax
  mov     rax, 50
  pop     r10
  cmp     r10, rax
  setg    al
  movzx   rax, al
  cmp     rax, 0
  je      .L21
  jmp     .L20
.L21:
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-28]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  jmp     .L19
.L20:
  mov     eax, dword [rbp-4]
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret
