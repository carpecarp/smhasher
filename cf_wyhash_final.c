/* 
 * This is free and unencumbered software released into the public domain.
 * 
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 * 
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 * For more information, please refer to <http://unlicense.org/>
 *
 * author: 王一 Wang Yi <godspeed_china@yeah.net>
 * contributors: Reini Urban, Dietrich Epp, Joshua Haberman, Tommy Ettinger, Daniel Lemire, Otmar Ertl, cocowalla, leo-yuriev, Diego Barrios Romero, paulie-g, dumblob, Yann Collet, ivte-ms, hyb, James Z.M. Gao, easyaspi314 (Devin), TheOneric
 */

// this is based on wyhash_final_version_3 (for git tracking)
// vulnerable to bad seeds effectively WYHASH_CONDOM == 1

//includes
#include <stdint.h>
#include <string.h>

#include "cf_machdepmath.h"

static inline uint64_t
_wyrot(uint64_t x)
{
	return (x >> 32) | (x << 32);
}

static inline uint64_t
_wymix(uint64_t A, uint64_t B)
{
	cf_mult128(&A, &B);
	return A ^ B;
}

// read functions
static inline uint64_t
_wyr8(const uint8_t *p)
{
	return cf_ld8_to_le(p);
}


static inline uint64_t
_wyr4(const uint8_t *p)
{
	uint32_t v = cf_ld4_to_le(p);
	
	return v;
}

static inline uint64_t
_wyr3(const uint8_t *p, size_t k)
{
	// implicit little endian order when handling 1, 2  or 3 bytes
	return (((uint64_t)p[0]) << 16) | (((uint64_t)p[k >> 1]) << 8) | p[k - 1];
}

uint64_t _cf_wyhash(const void *key, int len, uint64_t seed, const uint64_t *secret);

uint64_t
cf_wyhash(const void *key, size_t len)
{
	static const uint64_t _wyp[4] = {0xa0761d6478bd642full,
		0xe7037ed1a0b428dbull, 0x8ebc6af09c88c6e3ull, 0x589965cc75374cc3ull};

	return _cf_wyhash(key, (int)(len & 0x7FFFFFFF), (uint64_t)0x29FBB14cc886f, _wyp);
}

// wyhash main function
uint64_t
_cf_wyhash(const void *key, int len, uint64_t seed, const uint64_t *secret)
{
	const uint8_t *p = (const uint8_t *)key;
	uint64_t a,	b;

	seed ^= *secret;	

	if (len <= 16) {
		if (len >= 4) {
			a = (_wyr4(p) << 32) | _wyr4(p + ((len >> 3) << 2));
			b = (_wyr4(p + len - 4) << 32) |
				_wyr4(p + len - 4 - ((len >> 3) << 2));
		}
		else if (len > 0) {
			a = _wyr3(p, len);
			b = 0;
		}
		else {
			a = b = 0;	
		}
	}
	else{
		size_t i = len;

		if (i > 48) {
			uint64_t see1 = seed, see2 = seed;

			do {
				seed = _wymix(_wyr8(p) ^ secret[1], _wyr8(p + 8) ^ seed);
				see1 = _wymix(_wyr8(p + 16) ^ secret[2], _wyr8(p + 24) ^ see1);
				see2 = _wymix(_wyr8(p + 32) ^ secret[3], _wyr8(p + 40) ^ see2);
				p += 48;
				i -= 48;
			} while(i > 48);

			seed ^= see1 ^ see2;
		}

		while (i > 16) {
			seed = _wymix(_wyr8(p) ^ secret[1], _wyr8(p + 8) ^ seed);
			i -= 16;
			p += 16;
		}

		a = _wyr8(p + i - 16);
		b = _wyr8(p + i - 8);
	}
	return _wymix(secret[1] ^ len, _wymix(a ^ secret[1], b ^ seed));
}
