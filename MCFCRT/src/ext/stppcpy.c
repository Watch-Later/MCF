// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2017, LH_Mouse. All wrongs reserved.

#include "stppcpy.h"
#include "../env/expect.h"
#include "../env/xassert.h"
#include "../stdc/string/_sse3.h"
#include "rep_movs.h"

char *_MCFCRT_stppcpy(char *s1, char *es1, const char *restrict s2){
	_MCFCRT_ASSERT(s1 < es1);

	// 如果 arp 是对齐到字的，就不用考虑越界的问题。
	// 因为内存按页分配的，也自然对齐到页，并且也对齐到字。
	// 每个字内的字节的权限必然一致。
	register char *wp = s1;
	register const char *arp = (const char *)((uintptr_t)s2 & (uintptr_t)-32);
	__m128i xz[1];
	__MCFCRT_xmmsetz(xz);
//=============================================================================
#define LOOP_BODY(skip_, wp_part_, wp_full_)	\
	{	\
		__m128i xw[2];	\
		uint32_t mask;	\
		arp = __MCFCRT_xmmload_2(xw, arp, _mm_load_si128);	\
		mask = __MCFCRT_xmmcmp_21b(xw, xz, _mm_cmpeq_epi8);	\
		mask &= (skip_);	\
		ptrdiff_t dist = arp - (s2 + (es1 - s1) - 1);	\
		dist &= ~dist >> (sizeof(dist) * 8 - 1);	\
		mask |= ~((uint32_t)-1 >> dist);	\
		__builtin_prefetch(arp + 64, 0, 0);	\
		if(_MCFCRT_EXPECT_NOT(mask != 0)){	\
			unsigned shift = (unsigned)__builtin_ctzl(mask);	\
			wp = (wp_part_);	\
			*wp = 0;	\
			return wp;	\
		}	\
		wp = (wp_full_);	\
	}
//=============================================================================
	LOOP_BODY((uint32_t)-1 << ((const char *)s2 - arp),
	          (char *)_MCFCRT_rep_movsb(_MCFCRT_NULLPTR, (uint8_t *)wp, (const uint8_t *)s2, (size_t)(arp - 32 + shift - s2)),
	          (char *)_MCFCRT_rep_movsb(_MCFCRT_NULLPTR, (uint8_t *)wp, (const uint8_t *)s2, (size_t)(arp - s2)));
	if(((uintptr_t)wp & ~(uintptr_t)-16) == 0){
		for(;;){
			LOOP_BODY((uint32_t)-1,
			          (char *)_MCFCRT_rep_movsb(_MCFCRT_NULLPTR, (uint8_t *)wp, (const uint8_t *)arp - 32, shift),
			          __MCFCRT_xmmstore_2(wp, xw, _mm_store_si128));
		}
	} else {
		for(;;){
			LOOP_BODY((uint32_t)-1,
			          (char *)_MCFCRT_rep_movsb(_MCFCRT_NULLPTR, (uint8_t *)wp, (const uint8_t *)arp - 32, shift),
			          __MCFCRT_xmmstore_2(wp, xw, _mm_storeu_si128));
		}
	}
}