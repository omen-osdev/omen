#ifndef _STDARG
#define _STDARG
/* type definitions */
typedef char *va_list;
/* macros */
#define va_arg(ap, T) \
(* (T *)(((ap) += _Bnd(T, 3U)) - _Bnd(T, 3U)))
#define va_end(ap) (void)0
#define va_start(ap, A) \
(void)((ap) = (char *)&(A) + _Bnd(A, 3U))
#define _Bnd(X, bnd) ((sizeof (X) + (bnd)) & ~(bnd))
#define va_copy(dest, src) ((dest) = (src))
#endif