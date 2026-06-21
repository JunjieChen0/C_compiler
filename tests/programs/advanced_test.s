section .text

global add
add:
  push    rbp
  mov     rbp, rsp
  mov     qword [rbp-16], rdx
  mov     qword [rbp-8], rcx
  sub     rsp, 16
  mov     eax, dword [rbp-8]
  push    rax
  mov     eax, dword [rbp-16]
  pop     r10
  add     rax, r10
  jmp     .L0
.L0:
  mov     rsp, rbp
  pop     rbp
  ret

global factorial
factorial:
  push    rbp
  mov     rbp, rsp
  mov     qword [rbp-8], rcx
  sub     rsp, 16
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 1
  pop     r10
  cmp     r10, rax
  setle   al
  movzx   rax, al
  cmp     rax, 0
  je      .L2
  mov     rax, 1
  jmp     .L1
.L2:
  mov     eax, dword [rbp-8]
  push    rax
  mov     eax, dword [rbp-8]
  push    rax
  mov     rax, 1
  pop     r10
  sub     r10, rax
  mov     rax, r10
  push    rax
  pop     rcx
  call    n
  pop     r10
  imul    rax, r10
  jmp     .L1
.L1:
  mov     rsp, rbp
  pop     rbp
  ret

global main
main:
  push    rbp
  mov     rbp, rsp
  sub     rsp, 32
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-8]
  mov     rax, 10
  mov     dword [rbp-8], eax
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-4]
  mov     rax, 20
  mov     dword [rbp-4], eax
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-8]
  push    rax
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-4]
  push    rax
  pop     rdx
  pop     rcx
  call    p
  mov     dword [rbp-12], eax
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-8]
  push    rax
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-4]
  pop     r10
  cmp     r10, rax
  setg    al
  movzx   rax, al
  cmp     rax, 0
  je      .L5
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-8]
  jmp     .L6
.L5:
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-4]
.L6:
  mov     dword [rbp-16], eax
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-8]
  push    rax
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-4]
  pop     r10
  cmp     r10, rax
  setl    al
  movzx   rax, al
  cmp     rax, 0
  je      .L7
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-8]
  jmp     .L8
.L7:
  lea     rax, qword [rbp-8]
  mov     eax, dword [rbp-4]
.L8:
  mov     dword [rbp-20], eax
  mov     rax, 5
  push    rax
  pop     rcx
  call    factorial
  mov     dword [rbp-24], eax
  mov     eax, dword [rbp-12]
  push    rax
  mov     eax, dword [rbp-16]
  pop     r10
  add     rax, r10
  push    rax
  mov     eax, dword [rbp-20]
  pop     r10
  add     rax, r10
  push    rax
  mov     eax, dword [rbp-24]
  pop     r10
  add     rax, r10
  jmp     .L4
.L4:
  mov     rsp, rbp
  pop     rbp
  ret
