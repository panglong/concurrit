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

#include "concurrit.h"

namespace concurrit {


/********************************************************************************/

FILE* my_fopen(const char * filename, const char * mode);

void my_fclose(FILE* file);


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
		file_ = fopen(filename, flags);
		if(file_ == NULL) {
			printf("File could not be opened: %s", filename);
			bool CannotOpenFile = false;
			safe_assert(CannotOpenFile);
		}
	}

	Serializer(FILE* file) {
		safe_assert(file != NULL);
		file_ = file;
	}

	~Serializer() {
		safe_assert(file_ != NULL);
		fclose(file_);
	}

	template<typename T>
	void Store(T x) {
		int result = fwrite(&x, sizeof(T), 1, file_);
		safe_assert(result == 1);
	}

	template<typename T>
	T Load() {
		T x;
		int result = fread(&x, sizeof(T), 1, file_);
		if(result != 1) {
			if(feof(file_)) {
				throw new EOFException();
			} else {
				_Exit(UNRECOVERABLE_ERROR);
			}
		}
		return x;
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
	virtual void Load(FILE* file) {
		Serializer serializer(file);
		Load(&serializer);
	}
	virtual void Store(FILE* file) {
		Serializer serializer(file);
		Store(&serializer);
	}

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
