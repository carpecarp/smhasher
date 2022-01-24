/*
 * Copyright 2008-2018 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */



//==========================================================
// Includes.
//

#include <stddef.h>
#include <stdint.h>


//==========================================================
// Public API.
//

// murmurhash3_x64_128
#define ROTL(x, r) ((x << r) | (x >> (64 - r)))


static inline uint64_t
fmix64(uint64_t k)
{
	k ^= k >> 33;
	k *= 0xff51afd7ed558ccd;
	k ^= k >> 33;
	k *= 0xc4ceb9fe1a85ec53;
	k ^= k >> 33;

	return k;
}


void murmurHash3_x64_128_aero_cf(const uint8_t* key, const size_t len, uint8_t* out);

void MurmurHash3_x64_128_aero_cf( const void * key, const int len, const uint32_t seed, void * out )
{
    return murmurHash3_x64_128_aero_cf((const uint8_t*) key, (const size_t) len, (uint8_t*) out);
}

void
murmurHash3_x64_128_aero_cf(const uint8_t* key, const size_t len, uint8_t* out)
{
	const uint8_t* data = (const uint8_t*)key;
	const size_t nblocks = len / 16;

	uint64_t h1 = 0;
	uint64_t h2 = 0;

	const uint64_t c1 = 0x87c37b91114253d5;
	const uint64_t c2 = 0x4cf5ad432745937f;

	//----------
	// body

	const uint64_t* blocks = (const uint64_t*)(data);

	for (size_t i = 0; i < nblocks; i++) {
		uint64_t k1 = blocks[i * 2 + 0];
		uint64_t k2 = blocks[i * 2 + 1];

		k1 *= c1;
		k1 = ROTL(k1, 31);
		k1 *= c2; h1 ^= k1;

		h1 = ROTL(h1, 27);
		h1 += h2;
		h1 = h1 * 5+0x52dce729;

		k2 *= c2;
		k2 = ROTL(k2, 33);
		k2 *= c1; h2 ^= k2;

		h2 = ROTL(h2, 31);
		h2 += h1;
		h2 = h2 * 5+0x38495ab5;
	}

	//----------
	// tail

	const uint8_t* tail = (const uint8_t*)(data + nblocks * 16);

	uint64_t k1 = 0;
	uint64_t k2 = 0;

	switch (len & 15) {
	case 15:
		k2 ^= ((uint64_t)tail[14]) << 48;
		// No break.
	case 14:
		k2 ^= ((uint64_t)tail[13]) << 40;
		// No break.
	case 13:
		k2 ^= ((uint64_t)tail[12]) << 32;
		// No break.
	case 12:
		k2 ^= ((uint64_t)tail[11]) << 24;
		// No break.
	case 11:
		k2 ^= ((uint64_t)tail[10]) << 16;
		// No break.
	case 10:
		k2 ^= ((uint64_t)tail[9]) << 8;
		// No break.
	case  9:
		k2 ^= ((uint64_t)tail[8]) << 0;

		k2 *= c2;
		k2 = ROTL(k2, 33);
		k2 *= c1;
		h2 ^= k2;

		// No break.
	case  8:
		k1 ^= ((uint64_t)tail[7]) << 56;
		// No break.
	case  7:
		k1 ^= ((uint64_t)tail[6]) << 48;
		// No break.
	case  6:
		k1 ^= ((uint64_t)tail[5]) << 40;
		// No break.
	case  5:
		k1 ^= ((uint64_t)tail[4]) << 32;
		// No break.
	case  4:
		k1 ^= ((uint64_t)tail[3]) << 24;
		// No break.
	case  3:
		k1 ^= ((uint64_t)tail[2]) << 16;
		// No break.
	case  2:
		k1 ^= ((uint64_t)tail[1]) << 8;
		// No break.
	case  1:
		k1 ^= ((uint64_t)tail[0]) << 0;

		k1 *= c1;
		k1 = ROTL(k1, 31);
		k1 *= c2;
		h1 ^= k1;
	}

	//----------
	// finalization

	h1 ^= len;
	h2 ^= len;

	h1 += h2;
	h2 += h1;

	h1 = fmix64(h1);
	h2 = fmix64(h2);

	h1 += h2;
	h2 += h1;

	((uint64_t*)out)[0] = h1;
	((uint64_t*)out)[1] = h2;
}

