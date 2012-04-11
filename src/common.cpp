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

#include "common.h"

namespace concurrit {

/********************************************************************************/

main_args StringToMainArgs(const std::string& s, bool add_program_name /*= false*/) {
	return StringToMainArgs(s.c_str(), add_program_name);
}

main_args StringToMainArgs(const char* s, bool add_program_name /*= false*/) {
	std::vector<char*> args;
	if(add_program_name) {
		args.push_back("<dummy_program_name>");
	}
	args.push_back(const_cast<char*>(s));
	return ArgVectorToMainArgs(args);
}

/********************************************************************************/

main_args ArgVectorToMainArgs(const std::vector<char*>& args) {
	const size_t c = args.size();
	main_args m(c, NULL);

	if(c > 0) {
		m.argv_ = new char*[c];
		for(int i = 0; i < c; ++i) m.argv_[i] = args[i];
	}

	return m;
}

/********************************************************************************/

void short_sleep(long nanoseconds, bool continue_on_signal) {
	safe_assert(0 <= nanoseconds && nanoseconds < 1000000000L);
	struct timespec tv;
	tv.tv_sec = (time_t) 0;
	tv.tv_nsec = nanoseconds;

	int rval = 0;
	do {
		rval = nanosleep(&tv, &tv);
		if(rval == EINVAL) {
			fprintf(stderr, "Invalid time value: %lu\n", nanoseconds);
			_Exit(UNRECOVERABLE_ERROR);
		}
		if(rval == EFAULT) {
			fprintf(stderr, "Problem with copying from user space: %lu\n", nanoseconds);
			_Exit(UNRECOVERABLE_ERROR);
		}
		safe_assert (rval == 0 || rval == EINTR);

	} while(rval == EINTR && continue_on_signal);
}

/********************************************************************************/

void print_stack_trace() {
	void *array[10];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);

	fprintf(stderr, "Stack trace (of %zd frames):\n", size);
	for (i = 0; i < size; i++)
		fprintf(stderr, "%s\n", strings[i]);

	free(strings);
}

/********************************************************************************/

void* FuncAddressByName(const char* name, bool default_first /*= true*/, bool try_other /*= false*/, bool fail_on_null /*= false*/) {
	bool next_first = !default_first;
	void* addr = reinterpret_cast<void*>(dlsym(next_first ? RTLD_NEXT : RTLD_DEFAULT, name));
	if(addr == NULL) {
		fprintf(stderr, "%s init of %s failed.\n", (next_first ? "RTLD_NEXT" : "RTLD_DEFAULT"), name);
		if(try_other) {
			addr = reinterpret_cast<void*>(dlsym(next_first ? RTLD_DEFAULT : RTLD_NEXT, name));
			if(addr == NULL) fprintf(stderr, "%s init of %s failed.\n", (next_first ? "RTLD_DEFAULT": "RTLD_NEXT"), name);
		}
	}
	if(addr == NULL && fail_on_null) {
		fprintf(stderr, "%s could not been found!\n", name);
		fflush(stderr);
		_Exit(UNRECOVERABLE_ERROR);
	}

	return addr;
}

/********************************************************************************/

} // end namespace


