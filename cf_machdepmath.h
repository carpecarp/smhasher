/* 
 * Copyright 2022 Aerospike, Inc.
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
#pragma once

#include <stdint.h>

#include "cf_byte_order.h"

static inline void
cf_mult128(uint64_t *A, uint64_t *B)
{

#if defined(__SIZEOF_INT128__)

	// safe for gcc, clang with ARM and X86_64
	__uint128_t r = *A;
	r *= *B;

	*A = (uint64_t)r;
	*B = (uint64_t)(r >> 64);

#elif defined(_MSC_VER) && defined(_M_X64)

	// Microsoft
	// has 128 bit native type, no combining

    #include <intrin.h>
    #pragma intrinsic(_umul128)

	*A = _umul128(*A, *B, B);

#else
	// very much reduced performance
	
	#warning "128-bit multiplication by hand"

	uint64_t ha = *A >> 32, hb = *B >> 32;
	uint64_t la = (uint32_t)*A, lb = (uint32_t)*B;
	uint64_t rh = ha * hb;
	uint64_t rm0 = ha * lb;
	uint64_t rm1 = hb * la;
	uint64_t rl = la * lb;
	uint64_t t = rl + (rm0 << 32); 
	uint64_t lo = t + (rm1 << 32);
	uint64_t c = (lo < t) + (t < rl);
	uint64_t hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;

	*A = lo;  *B = hi;
#endif
}

#define cf_ld8_to_le(p) cf_swap_to_le64(*(uint64_t*)p)
#define cf_ld4_to_le(p) cf_swap_to_le32(*(uint32_t*)p)
