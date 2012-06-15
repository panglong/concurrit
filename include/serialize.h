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

#ifndef SERIALIZE_H_
#define SERIALIZE_H_

#include "common.h"
#include "util.h"

namespace concurrit {


/********************************************************************************/

class SerializationException : public std::exception {
public:
	virtual const char* what() const throw() {
		return "Could not serialize!";
	}
};

/********************************************************************************/

class EOFException : public std::exception {
public:
	EOFException() throw() : std::exception() {}
	virtual const char* what() const throw() { return "EOF"; }
};

/********************************************************************************/

class Serializer {
public:
	Serializer(const char* filename, const char* flags) {
		file_ = my_fopen(filename, flags, EXIT_ON_FAIL);
		if(file_ == NULL) {
			safe_fail("File could not be opened: %s", filename);
		}
	}

	Serializer(FILE* file) {
		safe_assert(file != NULL);
		file_ = file;
	}

	~Serializer() {
		Close();
	}

	void Close() {
		safe_assert(file_ != NULL);
		my_fclose(file_, EXIT_ON_FAIL);
		file_ = NULL;
	}

	template<typename T>
	void Store(const T& x) {
		Store(&x);
	}

	template<typename T>
	void Store(T* x, int size) {
		safe_assert(size >= 0);
		for(int i = 0; i < size; ++i) {
			Store(&x[i]);
		}
	}

	template<typename T>
	void Store(T* x) {
		int result = fwrite(x, sizeof(T), 1, file_);
		if(result != 1) {
			safe_fail("Error while writing to file!\n");
		}
	}

	template<typename T>
	bool Load(T* x) {
		int result = fread(x, sizeof(T), 1, file_);
		if(result != 1) {
			if(feof(file_)) {
				return false;
			} else {
				safe_fail("Error while reading from file!\n");
			}
		}
		return true;
	}

	template<typename T>
	bool Load(T* x, int size) {
		safe_assert(size >= 0);
		for(int i = 0; i < size; ++i) {
			if(!Load(&x[i])) {
				return false;
			}
		}
		return true;
	}

	bool HasMore() {
		return feof(file_) == 0;
	}

private:
	DECL_FIELD(FILE*, file)
};

/********************************************************************************/

class Serializable {
public:
	virtual ~Serializable(){}

	void Load(const char* filename, const char* flags = "r");

	void Store(const char* filename, const char* flags = "w");

	void Load(FILE* file);

	void Store(FILE* file);

	virtual void Load(Serializer* serializer) = 0;
	virtual void Store(Serializer* serializer) = 0;
};

/********************************************************************************/

class Writable {
public:
	virtual ~Writable(){}
	virtual void ToStream(FILE* file) = 0;
};

/********************************************************************************/

} // end namespace

#endif /* SERIALIZE_H_ */
