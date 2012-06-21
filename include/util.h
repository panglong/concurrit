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

#ifndef UTIL_H_
#define UTIL_H_

namespace concurrit {

/********************************************************************************/

// common utility functions
// for non-template ones, see common.cpp for the implementations

/********************************************************************************/

template<typename T>
std::string to_string(T i) {
	std::stringstream s;
	s << i;
	return s.str();
}

template<typename T>
std::string vector_to_string(const std::vector<T>& v) {
	std::stringstream s;
	s << "[";
	const char* comma = "";
	for(typename std::vector<T>::iterator itr = v.begin(), end = v.end(); itr < end; ++itr) {
		s << comma << (*itr);
		comma = ", ";
	}
	s << "]";
	return s.str();
}

template<typename T>
std::string array_to_string(const T* v[], int sz) {
	std::stringstream s;
	s << "[";
	const char* comma = "";
	for(int i = 0; i < sz; ++i) {
		const T* t = v[i];
		if(t == NULL) {
			s << comma << "NULL";
		} else {
			s << comma << t;
		}
		comma = ", ";
	}
	s << "]";
	return s.str();
}

inline const char* bool_to_string(bool b) {
	return (b ? "T" : "F");
}

inline const char* int_to_boolstring(int b) {
	safe_assert(BETWEEN(-1, b, 1));
	return (b == 1 ? "T" : (b == 0 ? "F" : "U"));
}

/********************************************************************************/

#define EXIT_ON_FAIL	(true)
#define SKIP_ON_FAIL	(false)

FILE* my_fopen(const char * filename, const char * mode, bool exit_on_fail = true);
void my_fclose(FILE* file, bool exit_on_fail = true);

main_args StringToMainArgs(const std::string& s, bool add_program_name = false);
main_args StringToMainArgs(const char* s, bool add_program_name = false);

std::vector<std::string> TokenizeStringToVector(char* str, const char* tokens);

main_args ArgVectorToMainArgs(const std::vector<char*>& args);

void short_sleep(long nanoseconds, bool continue_on_signal);

void* FuncAddressByName(const char* name, bool default_first = true, bool try_other = false, bool fail_on_null = false);
void* FuncAddressByName(const char* name, void* handle, bool fail_on_null = false);

std::vector<std::string>* ReadLinesFromFile(const char* filename, std::vector<std::string>* lines = NULL, bool exit_on_fail = true, char comment = '#');

bool generate_random_bool();

#define format_string_buff_size	(256)
std::string format_string (const char* format, ...);

void my_write(int fd, const void *buff, size_t size);
void my_read(int fd, void *buff, size_t size);

} // end namespace


#endif /* UTIL_H_ */
