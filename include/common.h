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

#ifndef COMMON_H_
#define COMMON_H_

#include <cstdlib>
#include <cstdio>
#include <dlfcn.h>
#include <stdarg.h>
#include <sys/types.h>
#include <stdint.h>
#include <semaphore.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <exception>
#include <iostream>
#include <sstream>

// std libraries
#include <vector>
#include <map>
#include <set>

// google libraries
#include <glog/logging.h>

// boost libraries
//#include <boost/shared_ptr.hpp>

namespace counit {

/********************************************************************************/

#define RESTRICT
//__restrict__

/********************************************************************************/

#define USE(x) (x) = (x)

#define BETWEEN(x,y,z)	(((x) <= (y)) && ((y) <= (z)))

#define NULL_OR_NONEMPTY(x)	((x) == NULL || !((x)->empty()))

#define _AS(o, c)			dynamic_cast<c>(o)

#define INSTANCEOF(o, c)	(_AS(o,c) != NULL)

#define ASINSTANCEOF(o, c)	/*CHECK_NOTNULL*/(_AS(o,c))

/********************************************************************************/

#define DECL_FIELD(type, name) \
		protected: \
		type name##_; \
		public: \
		inline type& name() { return name##_; } \
		inline void set_##name(type value) { name##_ = value; } \
		private:	\


#define DECL_FIELD_GET(type, name) \
		protected: \
		type name##_; \
		public: \
		inline type& name() { return name##_; } \
		private:	\

#define DECL_FIELD_REF(type, name) \
		protected: \
		type name##_; \
		public: \
		inline type* name() { return &name##_; } \
		inline void set_##name(type& value) { name##_ = value; } \
		private:	\


#define DECL_STATIC_FIELD(type, name) \
		protected: \
		static type name##_; \
		public: \
		static inline type name() { return name##_; } \
		static inline void set_##name(type value) { name##_ = value; } \
		private:	\

#define DECL_STATIC_FIELD_REF(type, name) \
		protected: \
		static type name##_; \
		public: \
		static inline type* name() { return &name##_; } \
		static inline void set_##name(type& value) { name##_ = value; } \
		private:	\

#define DECL_VOL_FIELD(type, name) \
		protected: \
		volatile type name##_; \
		public: \
		inline type name() { return name##_; } \
		inline void set_##name(type value) { name##_ = value; } \
		private:	\

/********************************************************************************/

#define DISALLOW_COPY_AND_ASSIGN(type) \
		type(const type&);   \
		void operator=(const type&);

/********************************************************************************/

#define UNRECOVERABLE_ERROR 5
#define safe_assert(cond) \
		if (!(cond))  { \
			printf("\nCounit: safe assert fail: safe_assert(%s):", #cond); \
			printf(" \n\tfunction: %s\n\tfile: %s\n\tline: %d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
			fflush(stdout); \
			_Exit(UNRECOVERABLE_ERROR); \
		}

#define unimplemented() \
		VLOG(2) << "Executing unimplemented code in function: " << __PRETTY_FUNCTION__ << " file: " << __FILE__; \
		throw "Unimplemented"

/********************************************************************************/

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

/********************************************************************************/

#ifndef THREADID
typedef int THREADID;
#endif

#ifndef ADDRINT
typedef uintptr_t ADDRINT;
#endif

extern const char* CONCURRIT_HOME;

#define InWorkDir(f)	((std::string(CHECK_NOTNULL(CONCURRIT_HOME)) + "/work/" + f).c_str())


} // end namepace

#endif /* COMMON_H_ */
