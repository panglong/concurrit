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
#include <time.h>
#include <sys/time.h>
#include <exception>
#include <iostream>
#include <sstream>
#include <limits.h>
#include <climits>

// std libraries
#include <vector>
#include <map>
#include <set>
#include <stack>

#ifdef __linux__
#define LINUX
#endif

namespace concurrit {

/********************************************************************************/

#ifndef RESTRICT
#define RESTRICT
//__restrict__
#endif

#ifndef INLINE
#define INLINE inline
#endif

#ifndef UNUSED
#define UNUSED __attribute__((unused))
#endif

#ifndef USE
#define USE(x) ((void)(x))
#endif

#if defined(XP_OS2) || (defined(__GNUC__) && defined(__i386))
#define BREAK_HERE()	asm("int $3") // __asm { int 3 }
#else
#define BREAK_HERE()
#endif

/********************************************************************************/

#ifndef	TRUE
#define TRUE	1
#endif	/* TRUE */

#ifndef	FALSE
#define FALSE	0
#endif	/* FALSE */

/********************************************************************************/

#define BETWEEN(x,y,z)		(((x) <= (y)) && ((y) <= (z)))
#define BETWEEN2(x,y1,y2,z)	(((x) <= (y1)) && (y1 <= y2) && ((y2) <= (z)))

#define NULL_OR_NONEMPTY(x)	((x) == NULL || !((x)->empty()))

#define _AS(o, c)			dynamic_cast<c>(o)

#define INSTANCEOF(o, c)	(_AS(o,c) != NULL)

#define ASINSTANCEOF(o, c)	(_AS(o,c))

/********************************************************************************/

#define DECL_FIELD(type, name) \
		protected: \
		type name##_; \
		public: \
		inline type& name() { return name##_; } \
		inline void set_##name(type value) { name##_ = value; } \
		private:	\

#define DECL_FIELD_CONST(type, name) \
		protected: \
		type name##_; \
		public: \
		inline type name() const { return name##_; } \
		inline void set_##name(type value) { name##_ = value; } \
		private:	\

#define DECL_FIELD_GET(type, name) \
		protected: \
		type name##_; \
		public: \
		inline type& name() { return name##_; } \
		private:	\

#define DECL_FIELD_SET(type, name) \
		protected: \
		type name##_; \
		public: \
		inline void set_##name(type value) { name##_ = value; } \
		private:	\


#define DECL_FIELD_REF(type, name) \
		protected: \
		type name##_; \
		public: \
		inline type* name() { return &name##_; } \
		inline void set_##name(type& value) { name##_ = value; } \
		private:	\

#define DECL_FIELD_GET_REF(type, name) \
		protected: \
		type name##_; \
		public: \
		inline type* name() { return &name##_; } \
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
		inline type name() volatile { do { return name##_; } while(false); } \
		inline void set_##name(type value) { do { name##_ = value; } while(false); } \
		private:	\

/********************************************************************************/

#define DISALLOW_COPY_AND_ASSIGN(type) \
		type(const type&);   \
		void operator=(const type&);

/********************************************************************************/

#define UNRECOVERABLE_ERROR 5

#include <execinfo.h>
void print_stack_trace();

#define safe_exit(c)	fprintf(stderr, "Terminating with exit-code: %s.\n", #c); _Exit(c)
#define safe_fail(...) 	fprintf(stderr, __VA_ARGS__); fprintf(stderr, " \n\tfunction: %s\n\tfile: %s\n\tline: %d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); raise(SIGTERM);
#define safe_check(cond, ...) 	if (!(cond))  { safe_fail("\nCounit: safe check fail: safe_check(%s):", #cond); }

#ifdef SAFE_ASSERT
#define NDEBUG
#define MYLOG(c)			VLOG(c)
#define MYLOG_IF(c, p)		VLOG_IF(c, p)
#define safe_notnull(o) 	(CHECK_NOTNULL(o))
#define safe_assert(cond) 	if (!(cond))  { safe_fail("\nCounit: safe assert fail: safe_assert(%s):", #cond); }
#else
#undef NDEBUG
#define GOOGLE_STRIP_LOG	1
#define MYLOG(c)			DLOG(INFO)
#define MYLOG_IF(c, p)		DLOG_IF(c, p)
#define safe_notnull(o) 	(o)
#define safe_assert(cond) /* noop */
#endif

#define safe_delete(o) 		delete (safe_notnull(o))

#define unimplemented() safe_fail("Executing unimplemented code in function: %s, file %s!\n", __PRETTY_FUNCTION__, __FILE__);

#define unreachable() 	safe_fail("Executing unreachable code in function: %s, file %s!\n", __PRETTY_FUNCTION__, __FILE__);

/********************************************************************************/
// #define NOTNULL(ptr)	(ptr)

template<typename T>
T* NOTNULL(T* ptr) {
	safe_assert(ptr != NULL);
	return ptr;
}

/********************************************************************************/

#ifndef THREADID
typedef int THREADID;
#endif

const THREADID INVALID_THREADID = THREADID(-1);

#ifndef ADDRINT
typedef uintptr_t ADDRINT;
#endif

#define SIZEOF_ASSERT(e) typedef char __SIZEOF_ASSERT__[(e)?1:-1]
SIZEOF_ASSERT(sizeof(ADDRINT) == sizeof(void*));

inline ADDRINT	PTR2ADDRINT(void* p)	{ return (reinterpret_cast<ADDRINT>(p)); }
inline void* 	ADDRINT2PTR(ADDRINT p)	{ return (reinterpret_cast<void*>(p)); }

/********************************************************************************/

#ifndef vctime_t
typedef unsigned int vctime_t;
#endif

/********************************************************************************/

typedef int (*MainFuncType) (int, char**);
typedef void* (*ThreadFuncType) (void*);

/********************************************************************************/

struct main_args {
	int argc_;
	char** argv_;
	main_args(int argc = 0, char** argv = NULL) : argc_(argc), argv_(argv) {}
	~main_args() {} // { if(argv_ != NULL) delete argv_; }

	std::string ToString();

	bool check() {
		return (argc_ == 0 && argv_ == NULL) || (argc_ > 0 && argv_ != NULL);
	}
};

/********************************************************************************/

#define USECSPERSEC	999999L

/********************************************************************************/

} // end namepace

#endif /* COMMON_H_ */
