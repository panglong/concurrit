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


template<>
void Serializer::Store<std::string>(std::string* x) {
	const char* s = x->c_str();
	int len = x->length();
	if(len >= 256 || s[len] != '\0') safe_fail("Invalid string!\n");
	Store<int>(len);
	int result = fwrite(s, sizeof(char), len, file_);
	if(result != len) safe_fail("Error while writing to file!\n");
}

/********************************************************************************/

template<>
bool Serializer::Load<std::string>(std::string* str) {
	char s[256];
	int len = 0;
	if(!Load<int>(&len)) safe_fail("Error when reading length of string from file!");
	safe_assert(len < 256);
	int result = fread(s, sizeof(char), len, file_);
	if(result != len) {
		if(feof(file_)) {
			return false;
		} else {
			safe_fail("Error while reading from file!\n");
		}
	}
	s[len] = '\0';
	str->assign(s, len);
	return true;
}

/********************************************************************************/

template<>
void Serializer::Store<void*>(void** x) {
	ADDRINT a = PTR2ADDRINT(*x);
	Store<ADDRINT>(a);
}

template<>
bool Serializer::Load<void*>(void** x) {
	ADDRINT a = 0;
	bool r = Load<ADDRINT>(&a);
	if(r) *x = ADDRINT2PTR(a);
	return r;
}

/********************************************************************************/

void Serializable::Load(const char* filename, const char* flags /*= "r"*/) {
	Serializer serializer(filename, flags);
	Load(&serializer);
}

void Serializable::Store(const char* filename, const char* flags /*= "w"*/) {
	Serializer serializer(filename, flags);
	Store(&serializer);
}

void Serializable::Load(FILE* file) {
	Serializer serializer(file);
	Load(&serializer);
}
void Serializable::Store(FILE* file) {
	Serializer serializer(file);
	Store(&serializer);
}

/********************************************************************************/

} // end namespace
