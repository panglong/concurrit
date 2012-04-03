/**
 * Copyright (c) 2010-2011,
 * Tayfun Elmas    <elmas@cs.berkeley.edu>
 * All rights reserved.
 * <p/>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * <p/>
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * <p/>
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * <p/>
 * 3. The names of the contributors may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 * <p/>
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef STR_H_
#define STR_H_

#include "common.h"

namespace concurrit {

/*
 * Definitions for fixed-length strings
 */
static const unsigned int MAX_STR_LEN = 16;

typedef char str_t[MAX_STR_LEN];

static inline void str_copy(str_t to, str_t from) {
	safe_assert(from != NULL);
	safe_assert(to != NULL);
	safe_assert(strlen(from) < MAX_STR_LEN);

	strncpy(to, from, MAX_STR_LEN);
}

static inline void str_copy(str_t to, const char* from) {
	safe_assert(from != NULL);
	safe_assert(to != NULL);
	safe_assert(strlen(from) < MAX_STR_LEN);

	strncpy(to, from, MAX_STR_LEN);
}

static inline void str_copy(str_t to, std::string& from) {
	safe_assert(to != NULL);
	const char* fromc = from.c_str();
	safe_assert(strlen(fromc) < MAX_STR_LEN);

	strncpy(to, fromc, MAX_STR_LEN);
}

} // end namespace


#endif /* STR_H_ */
