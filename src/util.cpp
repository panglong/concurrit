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

#include "concurrit.h"

namespace concurrit {

/********************************************************************************/

std::string main_args::ToString() {
		return array_to_string(const_cast<const char**>(argv_), argc_);
	}

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
		for(size_t i = 0; i < c; ++i) m.argv_[i] = args[i];
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
			safe_fail("Invalid time value: %lu\n", nanoseconds);
		}
		if(rval == EFAULT) {
			safe_fail("Problem with copying from user space: %lu\n", nanoseconds);
		}
		safe_assert (rval == 0 || rval == EINTR);

	} while(rval == EINTR && continue_on_signal);
}

/********************************************************************************/

void print_stack_trace() {
	void *array[32];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace (array, 32);
	strings = backtrace_symbols (array, size);

	fprintf(stderr, "Stack trace (of %zd frames):\n", size);
	for (i = 0; i < size; i++)
		fprintf(stderr, "%s\n", strings[i]);

	free(strings);
}

/********************************************************************************/

void* FuncAddressByName(const char* name, bool default_first /*= true*/, bool try_other /*= false*/, bool fail_on_null /*= false*/) {
	void* first_handle = default_first ? RTLD_DEFAULT : RTLD_NEXT;
	void* addr = FuncAddressByName(name, first_handle, try_other ? false : fail_on_null);

	if(addr == NULL && try_other) {
		void* second_handle = default_first ? RTLD_NEXT : RTLD_DEFAULT;
		addr = FuncAddressByName(name, second_handle, fail_on_null);
	}

	safe_assert(addr != NULL || !fail_on_null);
	return addr;
}

/********************************************************************************/

void* FuncAddressByName(const char* name, void* handle, bool fail_on_null /*= false*/) {
	void* addr = reinterpret_cast<void*>(dlsym(handle, name));
	if(addr == NULL) {
		if(fail_on_null) {
			safe_fail("dlsym init of %s failed.\n", name);
		}
	}
	return addr;
}

/********************************************************************************/

std::vector<std::string>* ReadLinesFromFile(const char* filename, std::vector<std::string>* lines /*= NULL*/, bool exit_on_fail /*= true*/, char comment /*= '#'*/) {
	if(lines == NULL) {
		lines = new std::vector<std::string>();
	}
	const size_t buff_sz = 256;
	FILE* fin = my_fopen(filename, "r", exit_on_fail);

	if(fin != NULL) {
		char buff[buff_sz];
		while(!feof(fin)) {
			if(fgets(buff, buff_sz, fin) == NULL) {
				break;
			} else {
				size_t sz = strnlen(buff, buff_sz);
				if(sz > 0 && buff[0] != comment) {
					if(buff[sz-1] == '\n') {
						buff[sz-1] = '\0';
						--sz;
						if(sz == 0) continue;
					}

					lines->push_back(buff);
				}
			}
		}
		my_fclose(fin, exit_on_fail);
	}
	return lines;
}

/********************************************************************************/

FILE* my_fopen(const char * filename, const char * mode, bool exit_on_fail /*= true*/) {
	FILE* file = fopen(filename, mode);
	if(file == NULL && exit_on_fail) {
		safe_fail("File could not be opened: %s\n", filename);
	}
	return file;
}

/********************************************************************************/

void my_fclose(FILE* file, bool exit_on_fail /*= true*/) {
	if(fclose(file) && exit_on_fail) {
		safe_fail("File could not be closed\n");
	}
}

/********************************************************************************/
void my_write(int fd, const void *buff, size_t size) {
	ssize_t offset = 0;
	while(size > 0) {
		ssize_t sz = write(fd, buff + offset, size);
		safe_check(sz >= 0);
		size = size - sz;
		offset = offset + sz;
	}
}

/********************************************************************************/

void my_read(int fd, void *buff, size_t size) {
	ssize_t offset = 0;
	while(size > 0) {
		ssize_t sz = read(fd, buff + offset, size);
		safe_check(sz >= 0);
		size = size - sz;
		offset = offset + sz;
	}
}

/********************************************************************************/

bool generate_random_bool() {
	static MTRand rn_gen;
	return (rn_gen.randInt(uint32_t(1024)) > uint32_t(512));
}

/********************************************************************************/

std::vector<std::string> TokenizeStringToVector(char* str, const char* tokens) {
	std::vector<std::string> strlist;
	char * pch;
	pch = strtok (str, tokens);
	while (pch != NULL)
	{
		strlist.push_back(std::string(pch));
		pch = strtok (NULL, tokens);
	}
	return strlist;
}

/********************************************************************************/

std::string format_string (const char* format, ...) {
  char buff[format_string_buff_size];
  va_list args;
  va_start (args, format);
  vsnprintf (buff, format_string_buff_size, format, args);
  va_end (args);
  return std::string(buff);
}

/********************************************************************************/

} // end namespace


