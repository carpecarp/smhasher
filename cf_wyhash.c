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

#define WYHASH_BAD_SEEDS 1
#ifdef WYHASH_BAD_SEEDS
//protections that produce different results:
//1: normal valid behavior
//2: extra protection against entropy loss (probability=2^-63), aka. "blind multiplication"
//   There are 2 known bad seeds for the 32bit and many for the 64bit variants.
//   Recommended is just to skip or inc those known bad seeds, if you dont want to set WYHASH_CONDOM 2
//     32bit: 429dacdd, d637dbf3
//     64bit: *14cc886e, *1bf4ed84
#define WYHASH_CONDOM 1
#else
// not vulnerable to bad seeds. the strict variant.
#define WYHASH_CONDOM 2
void wyhash_condom_test_cf (const void * key, int len, uint32_t seed, void * out) {
  *(uint64_t*)out = wyhash(key, (uint64_t)len, (uint64_t)seed, _wyp);
}
#endif


/* quick example:
   string s = "fjsakfdsjkf";
   uint64_t hash = wyhash(s.c_str(), s.size(), 0, _wyp);
*/

// this is based on wyhash_final_version_3 (for git tracking)

#ifndef WYHASH_32BIT_MUM
//0: normal version, slow on 32 bit systems
//1: faster on 32 bit systems but produces different results, incompatible with wy2u0k function
#define WYHASH_32BIT_MUM 0  
#endif

//includes
#include <stdint.h>
#include <string.h>
#if defined(_MSC_VER) && defined(_M_X64)
  #include <intrin.h>
  #pragma intrinsic(_umul128)
#endif

//likely and unlikely macros
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
  #define _likely_(x)  __builtin_expect(x,1)
  #define _unlikely_(x)  __builtin_expect(x,0)
#else
  #define _likely_(x) (x)
  #define _unlikely_(x) (x)
#endif

//128bit multiply function
static inline uint64_t _wyrot(uint64_t x) { return (x>>32)|(x<<32); }
static inline void _wymum(uint64_t *A, uint64_t *B)
{
#if(WYHASH_32BIT_MUM)
	uint64_t hh = (*A>>32)*(*B>>32), hl = (*A>>32)*(uint32_t)*B, lh = (uint32_t)*A*(*B>>32), ll = (uint64_t)(uint32_t)*A*(uint32_t)*B;
#if(WYHASH_CONDOM>1)
	*A^ = _wyrot(hl)^hh; *B^ = _wyrot(lh)^ll;
#else
	*A = _wyrot(hl)^hh; *B = _wyrot(lh)^ll;
#endif
#elif defined(__SIZEOF_INT128__)
// !WYHASH_32BIT_MUM
	__uint128_t r = *A; r *= *B;
#if(WYHASH_CONDOM>1)
	*A ^= (uint64_t)r; *B ^= (uint64_t)(r>>64);
#else
	*A = (uint64_t)r; *B = (uint64_t)(r>>64);
#endif
#elif defined(_MSC_VER) && defined(_M_X64)
#if(WYHASH_CONDOM>1)
// !__SIZEOF_INT128__ && !WYHASH_32BIT_NUM
// obviously multiplying two 64bit int and combining the result
	uint64_t  a,  b;
	a = _umul128(*A,*B,&b);
	*A ^= a;  *B ^= b;
#else
	// has 128 bit native type, no combining
	*A = _umul128(*A,*B,B);
#endif
#else
	// multiplication by hand
	uint64_t ha = *A>>32, hb = *B>>32, la = (uint32_t)*A, lb = (uint32_t)*B, hi, lo;
	uint64_t rh = ha*hb, rm0 = ha*lb, rm1 = hb*la, rl = la*lb, t = rl+(rm0<<32), c = t<rl;
	lo = t+(rm1<<32); c+ = lo<t; hi = rh+(rm0>>32)+(rm1>>32)+c;
#if(WYHASH_CONDOM>1)
	*A ^= lo;  *B ^= hi;
#else
	*A = lo;  *B = hi;
#endif
#endif
}

// multiply and xor mix function, aka MUM
static inline uint64_t _wymix(uint64_t A, uint64_t B){ _wymum(&A,&B); return A^B; }

// endian macros
#ifndef WYHASH_LITTLE_ENDIAN
  #if defined(_WIN32) || defined(__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    #define WYHASH_LITTLE_ENDIAN 1
  #elif defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #define WYHASH_LITTLE_ENDIAN 0
  #else
    #warning could not determine endianness! Falling back to little endian.
    #define WYHASH_LITTLE_ENDIAN 1
  #endif
#endif

// read functions
#if (WYHASH_LITTLE_ENDIAN)
static inline uint64_t _wyr8(const uint8_t *p) { uint64_t v; memcpy(&v, p, 8); return v;}
static inline uint64_t _wyr4(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v;}
#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
static inline uint64_t _wyr8(const uint8_t *p) { uint64_t v; memcpy(&v, p, 8); return __builtin_bswap64(v);}
static inline uint64_t _wyr4(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return __builtin_bswap32(v);}
#elif defined(_MSC_VER)
static inline uint64_t _wyr8(const uint8_t *p) { uint64_t v; memcpy(&v, p, 8); return _byteswap_uint64(v);}
static inline uint64_t _wyr4(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return _byteswap_ulong(v);}
#else
static inline uint64_t _wyr8(const uint8_t *p) {
  uint64_t v; memcpy(&v, p, 8);
  return (((v >> 56) & 0xff)| ((v >> 40) & 0xff00)| ((v >> 24) & 0xff0000)| ((v >>  8) & 0xff000000)| ((v <<  8) & 0xff00000000)| ((v << 24) & 0xff0000000000)| ((v << 40) & 0xff000000000000)| ((v << 56) & 0xff00000000000000));
}
static inline uint64_t _wyr4(const uint8_t *p) {
  uint32_t v; memcpy(&v, p, 4);
  return (((v >> 24) & 0xff)| ((v >>  8) & 0xff00)| ((v <<  8) & 0xff0000)| ((v << 24) & 0xff000000));
}
#endif
static inline uint64_t _wyr3(const uint8_t *p, size_t k) { return (((uint64_t)p[0])<<16)|(((uint64_t)p[k>>1])<<8)|p[k-1];}

#ifdef __cplusplus
// skip bad seeds with WYHASH_CONDOM 1
static void wyhash_seed_init(size_t &seed) {
  if ((seed & 0x14cc886e) || (seed & 0xd637dbf3))
    seed++;
}
static void wyhash32_seed_init(uint32_t &seed) {
  if ((seed == 0x429dacdd) || (seed & 0xd637dbf3))
    seed++;
}
#endif

// wyhash main function
uint64_t
wyhash_cf(const void *key, size_t len, uint64_t seed, const uint64_t *secret)
{
	const uint8_t *p = (const uint8_t *)key;
	uint64_t a,	b;

	seed ^= *secret;	

	if (_likely_(len <= 16)) {
		if (_likely_(len >= 4)) {
			a = (_wyr4(p) << 32) | _wyr4(p + ((len >> 3) << 2));
			b = (_wyr4(p + len - 4) << 32) |
				_wyr4(p + len - 4 - ((len >> 3) << 2));
		}
		else if (_likely_(len > 0)) {
			a = _wyr3(p, len);
			b = 0;
		}
		else {
			a = b = 0;	
		}
	}
	else{
		size_t i = len;

		if (_unlikely_(i > 48)) {
			uint64_t see1 = seed, see2 = seed;

			do {
				seed = _wymix(_wyr8(p) ^ secret[1], _wyr8(p + 8) ^ seed);
				see1 = _wymix(_wyr8(p + 16) ^ secret[2], _wyr8(p + 24) ^ see1);
				see2 = _wymix(_wyr8(p + 32) ^ secret[3], _wyr8(p + 40) ^ see2);
				p += 48; i -= 48;
			} while(_likely_(i > 48));

			seed ^= see1 ^ see2;
		}

		while (_unlikely_(i > 16)) {
			seed = _wymix(_wyr8(p) ^ secret[1], _wyr8(p + 8) ^seed);
			i -= 16;
			p += 16;
		}

		a = _wyr8(p + i - 16);
		b = _wyr8(p + i - 8);
	}
	return _wymix(secret[1] ^ len, _wymix(a ^ secret[1], b ^ seed));
}

//the default secret parameters
static const uint64_t _wyp[4] = {0xa0761d6478bd642full, 0xe7037ed1a0b428dbull, 0x8ebc6af09c88c6e3ull, 0x589965cc75374cc3ull};
