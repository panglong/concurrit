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

void print_stack_trace();

#ifdef SAFE_ASSERT
#include <execinfo.h>
#define safe_notnull(o) 	(CHECK_NOTNULL(o))
#define safe_assert(cond) \
		if (!(cond))  { \
			fprintf(stderr, "\nCounit: safe assert fail: safe_assert(%s):", #cond); \
			fprintf(stderr, " \n\tfunction: %s\n\tfile: %s\n\tline: %d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
			concurrit::print_stack_trace(); \
			fflush(stderr); \
			_Exit(UNRECOVERABLE_ERROR); \
		}
#else
#define safe_notnull(o) 	(o)
#define safe_assert(cond) /* noop */
#endif

#define safe_delete(o) 		delete (safe_notnull(o))

#define unimplemented() \
		VLOG(2) << "Executing unimplemented code in function: " << __PRETTY_FUNCTION__ << " file: " << __FILE__; \
		fprintf(stderr, "Unimplemented operation!"); \
		fflush(stderr); \
		_Exit(UNRECOVERABLE_ERROR); \

#define unreachable() \
		VLOG(2) << "Executing unreachable code in function: " << __PRETTY_FUNCTION__ << " file: " << __FILE__; \
		fprintf(stderr, "Unreachable code!"); \
		fflush(stderr); \
		_Exit(UNRECOVERABLE_ERROR); \

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

#ifndef ADDRINT
typedef uintptr_t ADDRINT;
#endif

#define PTR2ADDRINT(p)	(reinterpret_cast<ADDRINT>(p))
#define ADDRINT2PTR(p)	(reinterpret_cast<void*>(p))

/********************************************************************************/

#ifndef vctime_t
typedef unsigned int vctime_t;
#endif

/********************************************************************************/

const long MaxWaitTimeUSecs = 3 * 999999L;

/********************************************************************************/

class Config {
public:
	static bool OnlyShowHelp;
//	static bool CanEnableDisablePinTool;
	static int ExitOnFirstExecution;
	static bool DeleteCoveredSubtrees;
	static char* SaveDotGraphToFile;
//	static bool RunUncontrolled;
	static void ParseCommandLine(int argc = -1, char **argv = NULL);
};

/********************************************************************************/

enum ExecutionMode { COOPERATIVE, PREEMPTIVE };

const ExecutionMode ConcurritExecutionMode = PREEMPTIVE;

/********************************************************************************/

typedef int (*MainFuncType) (int, char**);
extern "C" int __main__ (int, char**);

/********************************************************************************/

struct main_args {
	int argc_;
	char** argv_;
	main_args(int argc = 0, char** argv = NULL) : argc_(argc), argv_(argv) {}
	~main_args() {} // { if(argv_ != NULL) delete argv_; }

	std::string ToString() {
		std::stringstream s;
		s << "[";
		const char* comma = "";
		for(int i = 0; i < argc_; ++i) {
			s << comma << (argv_[i] == NULL ? "NULL" : argv_[i]);
			comma = ", ";
		}
		s << "]";
		return s.str();
	}
};

/********************************************************************************/

class Concurrit {
public:
	static void Init(int argc = -1, char **argv = NULL);
	static void Destroy();
	static volatile bool IsInitialized();
	static void* CallDriverMain(void*);
	static void SetupSignalHandler();
	static void SignalHandler(int sig_num, siginfo_t * info, void * ucontext);
private:
	static volatile bool initialized_;
	DECL_STATIC_FIELD_REF(main_args, driver_args)
	DECL_STATIC_FIELD(MainFuncType, driver_main)
};

/********************************************************************************/

class ConcurritInitializer {
public:
	ConcurritInitializer(int argc = -1, char **argv = NULL) {
		Concurrit::Init(argc, argv);
	}
	~ConcurritInitializer() {
		Concurrit::Destroy();
	}
};

/********************************************************************************/

// functions that are tracked by pin tool

extern "C" void EnablePinTool();
extern "C" void DisablePinTool();

extern "C" void ThreadRestart();

/********************************************************************************/

#include "dummy.h"

/********************************************************************************/

// common utility functions
// for non-template ones, see common.cpp for the implementations

/********************************************************************************/

template<typename T>
std::string to_string(T i) {
	std::stringstream out;
	out << i;
	return out.str();
}

template<typename T>
std::string vector_to_string(const std::vector<T>& v) {
	std::stringstream s;
	s << "[";
	const char* comma = "";
	for(int i = 0; i < v.size(); ++i) {
		s << comma << v[i];
		comma = ", ";
	}
	s << "]";
	return s.str();
}

/********************************************************************************/

main_args ArgVectorToMainArgs(const std::vector<char*>& args);

void short_sleep(long nanoseconds, bool continue_on_signal);

/********************************************************************************/

} // end namepace

#endif /* COMMON_H_ */
