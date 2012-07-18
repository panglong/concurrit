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

/*
 * Defines the API for adding yield points
 */

#ifndef YIELDAPI_H_
#define YIELDAPI_H_

#include "common.h"
#include "sharedaccess.h"

namespace concurrit {

/********************************************************************************/

class SchedulePoint;
extern SchedulePoint* yield(const char* label, SourceLocation* loc = NULL, SharedAccess* access = NULL, bool force = false);

/********************************************************************************/

#define NO_ACCESS NULL

#define READ(x) \
	new SharedAccess(READ_ACCESS, new MemoryCell<T>(x), #x)

#define WRITE(x) \
	new SharedAccess(WRITE_ACCESS, new MemoryCell<T>(x), #x)

#define YIELD(label, access) \
	yield(label, RECORD_SRCLOC(), access)

#define FORCE_YIELD(label, access) \
	yield(label, RECORD_SRCLOC(), access, /*force=*/true)

/********************************************************************************/
// yielding reads and writes

template<typename T>
class writer {
public:
	explicit writer(const char* label, SharedAccess* access, SourceLocation* loc, bool yield_before = true)
	: label_(label), access_(access), loc_(loc), yield_before_(yield_before) {}

	T& operator =(const T& value) {
		MemoryCell<T>* cell = access_->cell_as<T>();

		// we set the value of the memory cell even before the yield,
		// but do not perform the access yet
		// so yield implementation can access the value even before the actual access
		cell->set_value(value);

		if(yield_before_) yield(label_, loc_, access_);

		// perform the actual access
		T& val = cell->write(value);

		// notify scenario about this access
		NOTNULL(Coroutine::Current())->OnAccess(access_);

		if(!yield_before_) yield(label_, loc_, access_);

		return val;
	}
private:
	DECL_FIELD(const char*, label)
	DECL_FIELD(SharedAccess*, access)
	DECL_FIELD(SourceLocation*, loc)
	DECL_FIELD(bool, yield_before)
};

/********************************************************************************/

template<typename T>
class reader {
public:
	explicit reader(const char* label, SharedAccess* access, SourceLocation* loc, bool yield_before = true)
	: label_(label), access_(access), loc_(loc), yield_before_(yield_before) {}

	operator T() {
		MemoryCell<T>* cell = access_->cell_as<T>();

		// allows us to load the value to cell.value
		cell->read();

		if(yield_before_) yield(label_, loc_, access_);

		// perform the actual access
		T val = cell->read();

		// notify scenario about this access
		NOTNULL(Coroutine::Current())->OnAccess(access_);

		if(!yield_before_) yield(label_, loc_, access_);

		return val;
	}
private:
	DECL_FIELD(const char*, label)
	DECL_FIELD(SharedAccess*, access)
	DECL_FIELD(SourceLocation*, loc)
	DECL_FIELD(bool, yield_before)
};

/********************************************************************************/

template<typename T>
inline reader<T> yield_read(const char* label, T* mem, const char* expr, SourceLocation* loc) {
	SharedAccess* access = new SharedAccess(READ_ACCESS, new MemoryCell<T>(mem), expr);
	return reader<T>(label, access, loc, /*yield_before=*/ true); // makes the yield on T()
}

template<typename T>
inline reader<T> read_yield(const char* label, T* mem, const char* expr, SourceLocation* loc) {
	SharedAccess* access = new SharedAccess(READ_ACCESS, new MemoryCell<T>(mem), expr);
	return reader<T>(label, access, loc, /*yield_before=*/ false); // makes the yield on T()
}

template<typename T>
inline writer<T> yield_write(const char* label, T* mem, const char* expr, SourceLocation* loc) {
	SharedAccess* access = new SharedAccess(WRITE_ACCESS, new MemoryCell<T>(mem), expr);
	return writer<T>(label, access, loc, /*yield_before=*/ true); // makes the yield on operator=()
}

template<typename T>
inline writer<T> write_yield(const char* label, T* mem, const char* expr, SourceLocation* loc) {
	SharedAccess* access = new SharedAccess(WRITE_ACCESS, new MemoryCell<T>(mem), expr);
	return writer<T>(label, access, loc, /*yield_before=*/ false); // makes the yield on operator=()
}

#define YIELD_READ(label, x) \
	yield_read(label, (&(x)), #x, RECORD_SRCLOC())

#define YIELD_WRITE(label, x) \
	yield_write(label, (&(x)), #x, RECORD_SRCLOC())

#define READ_YIELD(label, x) \
	read_yield(label, (&(x)), #x, RECORD_SRCLOC())

#define WRITE_YIELD(label, x) \
	write_yield(label, (&(x)), #x, RECORD_SRCLOC())

/********************************************************************************/

} // end namespace


#endif /* YIELDAPI_H_ */
