// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2018, LH_Mouse. All wrongs reserved.

#ifndef __MCFCRT_ENV_XSETJMP_H_
#define __MCFCRT_ENV_XSETJMP_H_

#include "_crtdef.h"

_MCFCRT_EXTERN_C_BEGIN

// `__builtin_setjmp()` makes use of the first 5 elements.
// The additional 3 are reserved for future use.
typedef _MCFCRT_STD uintptr_t _MCFCRT_jmp_buf[5 + 3];

// See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59039> for the purpose of this wrapper.
__attribute__((__noreturn__)) extern void __MCFCRT_longjmp_wrapper(void **__env) _MCFCRT_NOEXCEPT;

#define _MCFCRT_SETJMP(__env_)    (__builtin_setjmp((void **)(__env_)))
#define _MCFCRT_LONGJMP(__env_)   (__MCFCRT_longjmp_wrapper((void **)(__env_)))

_MCFCRT_EXTERN_C_END

#endif
