section .text

global sum_to_n
sum_to_n:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     rax, 0
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
  add     rax, r10
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

global is_even
is_even:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 16
  mov     eax, dword [rbp-8]
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
  je      .L4
  mov     rax, 1
  jmp     .L3
.L4:
  mov     rax, 0
  jmp     .L3
.L3:
  mov     rsp, rbp
  pop     rbp
  ret

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 64
  mov     rax, 0
  mov     dword [rbp-4], eax
  mov     rax, 10
  mov     dword [rbp-8], eax
  mov     rax, 20
  mov     dword [rbp-12], eax
  mov     eax, dword [rbp-8]
  push    rax
  mov     eax, dword [rbp-12]
  pop     r10
  add     rax, r10
  mov     dword [rbp-16], eax
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-16]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-8]
  push    rax
  mov     eax, dword [rbp-12]
  pop     r10
  imul    rax, r10
  mov     dword [rbp-20], eax
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-20]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-12]
  push    rax
  mov     eax, dword [rbp-8]
  mov     r11, rax
  pop     rax
  cqo
  idiv    r11
  mov     dword [rbp-24], eax
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-24]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-12]
  push    rax
  mov     eax, dword [rbp-8]
  mov     r11, rax
  pop     rax
  cqo
  idiv    r11
  mov     rax, rdx
  mov     dword [rbp-28], eax
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-28]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 2
  mov     cl, al
  pop     rax
  shl     rax, cl
  mov     dword [rbp-32], eax
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-32]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-12]
  push    rax
  mov     rax, 1
  mov     cl, al
  pop     rax
  sar     rax, cl
  mov     dword [rbp-36], eax
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-36]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-8]
  push    rax
  mov     eax, dword [rbp-12]
  pop     r10
  and     rax, r10
  mov     dword [rbp-40], eax
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-40]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-8]
  push    rax
  mov     eax, dword [rbp-12]
  pop     r10
  or      rax, r10
  mov     dword [rbp-44], eax
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-44]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-8]
  push    rax
  mov     eax, dword [rbp-12]
  pop     r10
  xor     rax, r10
  mov     dword [rbp-48], eax
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-48]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     rax, 0
  mov     dword [rbp-52], eax
.L7:
  mov     eax, dword [rbp-52]
  push    rax
  mov     rax, 10
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  je      .L8
  mov     eax, dword [rbp-52]
  mov     eax, dword [rbp-52]
  push    rax
  mov     rax, 1
  pop     r10
  add     rax, r10
  mov     dword [rbp-52], eax
  jmp     .L7
.L8:
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-52]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     rax, 0
  mov     dword [rbp-56], eax
.L9:
  mov     eax, dword [rbp-56]
  mov     eax, dword [rbp-56]
  push    rax
  mov     rax, 1
  pop     r10
  add     rax, r10
  mov     dword [rbp-56], eax
.L10:
  mov     eax, dword [rbp-56]
  push    rax
  mov     rax, 5
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  jne     .L9
.L11:
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     eax, dword [rbp-56]
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  mov     eax, dword [rbp-60]
  mov     rax, 0
  mov     dword [rbp-60], eax
.L12:
  mov     eax, dword [rbp-60]
  push    rax
  mov     rax, 3
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  je      .L14
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     rax, 1
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
.L13:
  mov     eax, dword [rbp-60]
  mov     r10, rax
  add     rax, 1
  mov     dword [rbp-60], eax
  mov     rax, r10
  jmp     .L12
.L14:
  mov     eax, dword [rbp-4]
  push    rax
  mov     rax, 100
  pop     r10
  cmp     r10, rax
  setg    al
  movzx   rax, al
  cmp     rax, 0
  je      .L15
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     rax, 10
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
  jmp     .L16
.L15:
  mov     eax, dword [rbp-4]
  mov     eax, dword [rbp-4]
  push    rax
  mov     rax, 20
  pop     r10
  add     rax, r10
  mov     dword [rbp-4], eax
.L16:
  mov     eax, dword [rbp-4]
  jmp     .L6
.L6:
  mov     rsp, rbp
  pop     rbp
  ret
