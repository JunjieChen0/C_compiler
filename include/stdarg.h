#ifndef _STDARG_H
#define _STDARG_H

typedef char *va_list;

// Windows x64 ABI: parameters are saved at 8-byte intervals on the stack
// [rbp-8]: 1st param (rcx), [rbp-16]: 2nd param (rdx), etc.
// va_start sets ap to point to the next parameter after 'last'
// Since parameters grow downward, next param is at lower address
#define va_start(ap, last) ((ap) = (char *)&(last) - 8)
#define va_arg(ap, type) (*(type *)((ap) -= 8, (ap)))
#define va_end(ap) ((ap) = (char *)0)
#define va_copy(dest, src) ((dest) = (src))

#endif
