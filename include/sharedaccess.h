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

#ifndef SHAREDACCESS_H_
#define SHAREDACCESS_H_

#include "vc.h"

namespace counit {


// base class for MemoryCell templates
class MemoryCellBase {
public:
	MemoryCellBase() {}
	virtual ~MemoryCellBase() {}
	virtual MemoryCellBase* Clone() = 0;
	virtual void update_value(MemoryCellBase* other) = 0;
	virtual ADDRINT int_address() = 0;
	virtual std::string ToString() = 0;
};

/********************************************************************************/

template<typename T>
class MemoryCell : public MemoryCellBase {
public:
	MemoryCell(T* address, T value)
	: MemoryCellBase(), address_(address), value_(value) {}

	MemoryCell(T* address)
	: MemoryCellBase(), address_(address) {}

	~MemoryCell() {}

	MemoryCellBase* Clone() { return new MemoryCell<T>(address_, value_); }

	ADDRINT int_address() { return reinterpret_cast<ADDRINT>(address_); }

	virtual void update_value(MemoryCellBase* other) {
		MemoryCell<T>* _other = ASINSTANCEOF(other, MemoryCell<T>*);
		safe_assert(_other->address_ == this->address_);
		value_ = _other->value_;
	}

	std::string ToString() {
		std::stringstream s;
		s << "(" << int_address() << ":" << value_ << ")";
		return s.str();
	}

	T& read() { return value_ = (*address_); }
	T& write(const T& value) { return (*address_) = (value_ = value); }

private:
	DECL_FIELD(T*, address)
	DECL_FIELD(T, value)
};

/********************************************************************************/

typedef bool AccessType;
#define WRITE_ACCESS true
#define READ_ACCESS false
#define CONFLICTING(t1, t2) ((t1) || (t2))

class SharedAccess {
public:
	SharedAccess(AccessType type, MemoryCellBase* cell, const char* expr = "<unknown>", vctime_t time = 0)
	: type_(type), cell_(cell), time_(time), expr_(expr)
	{}

	~SharedAccess() {
		safe_assert(cell_ != NULL);
		delete cell_;
	}

	SharedAccess* Clone() {
		return new SharedAccess(type_, cell_->Clone(), expr_, time_);
	}

	std::string ToString() {
		std::stringstream s;
		s << (type_ == READ_ACCESS ? "Read(" : "Write(") << expr_ << " = " << mem() << ")@" << time_;
		return s.str();
	}

	inline bool is_read() { return type_ == READ_ACCESS; }
	inline bool is_write() { return type_ == WRITE_ACCESS; }

	inline bool conflicts_with(SharedAccess* that) {
		return this->is_write() || that->is_write();
	}

	template<typename T>
	MemoryCell<T>* cell_as() {
		safe_assert(INSTANCEOF(cell_,MemoryCell<T>*));
		return ASINSTANCEOF(cell_,MemoryCell<T>*);
	}

	ADDRINT mem() {
		return cell_->int_address();
	}

private:
	DECL_FIELD(AccessType, type)
	DECL_FIELD(MemoryCellBase*, cell)
	DECL_FIELD(vctime_t, time)
	DECL_FIELD(const char*, expr)
};

/********************************************************************************/

class SourceLocation {
public:

	SourceLocation(const char* filename, const char* funcname, int line, int column = 0)
	: filename_(filename), funcname_(funcname), column_(column), line_(line)
	{}

	SourceLocation(std::string filename, std::string funcname, int line, int column = 0)
	: filename_(filename), funcname_(funcname), column_(column), line_(line)
	{}

	~SourceLocation() {}

	std::string ToString() {
		std::stringstream s;
		if(IsUnknown()) {
			s << "<unknown>";
		} else {
			s << filename_ << "(" << line_ << ":" << column_<< "): " << funcname_;
		}
		return s.str();
	}

	SourceLocation* Clone() {
		return new SourceLocation(filename_, funcname_, line_, column_);
	}

	bool IsUnknown() {
		bool r = (line_ == 0);
		safe_assert(!r || column_ == 0);
		safe_assert(!r || filename_ == "");
		return r;
	}

	bool operator ==(const SourceLocation& loc) const {
		return (column_ == loc.column_) && (line_ == loc.line_) && (filename_ == loc.filename_);
	}

	bool operator !=(const SourceLocation& loc) const {
		return !(operator==(loc));
	}

private:
	DECL_FIELD(std::string, filename)
	DECL_FIELD(std::string, funcname)
	DECL_FIELD(int, column)
	DECL_FIELD(int, line)
};

/********************************************************************************/

struct AccessLocPair {
	SharedAccess* access_;
	SourceLocation* loc_;
	AccessLocPair(SharedAccess* access = NULL, SourceLocation* loc = NULL)
	: access_(access), loc_(loc) {}
};

} // end namespace

#endif /* SHAREDACCESS_H_ */
