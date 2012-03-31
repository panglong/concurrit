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


#ifndef TRANSINFO_H_
#define TRANSINFO_H_

#include "common.h"

namespace concurrit {

/********************************************************************************/

enum TransitionConst {
	TRANS_MEM_WRITE,
	TRANS_MEM_READ,
	TRANS_FUNC_CALL,
	TRANS_FUNC_RETURN,
	TRANS_FUNC_ENTER,
	TRANS_ENDING
};

// class storing information about a single transition
class TransitionInfo {
public:
	TransitionInfo(TransitionConst type):
		type_(type) {}
	virtual ~TransitionInfo(){}
private:
	DECL_FIELD(TransitionConst, type)
};

typedef std::vector<TransitionInfo> TransitionInfoList;

/********************************************************************************/

class EndingTransitionInfo : public TransitionInfo {
public:
	EndingTransitionInfo(): TransitionInfo(TRANS_ENDING){}
	~EndingTransitionInfo(){}
};

/********************************************************************************/

class MemAccessTransitionInfo : public TransitionInfo {
public:
	MemAccessTransitionInfo(TransitionConst type, void* addr, uint32_t size, SourceLocation* loc):
		TransitionInfo(type), addr_(addr), size_(size), loc_(loc) {}
	~MemAccessTransitionInfo(){}
private:
	DECL_FIELD(void*, addr)
	DECL_FIELD(uint32_t, size)
	DECL_FIELD(SourceLocation*, loc)
};

/********************************************************************************/

class FuncCallTransitionInfo : public TransitionInfo {
public:
	FuncCallTransitionInfo(void* addr, SourceLocation* loc_src, SourceLocation* loc_target = NULL, bool direct = false):
		TransitionInfo(TRANS_FUNC_CALL), addr_(addr), loc_src_(loc_src), loc_target_(loc_target), direct_(direct) {}
	~FuncCallTransitionInfo(){}
private:
	DECL_FIELD(void*, addr)
	DECL_FIELD(SourceLocation*, loc_src)
	DECL_FIELD(SourceLocation*, loc_target)
	DECL_FIELD(bool, direct)
};

/********************************************************************************/

class FuncReturnTransitionInfo : public TransitionInfo {
public:
	FuncReturnTransitionInfo(void* addr, SourceLocation* loc, ADDRINT retval):
		TransitionInfo(TRANS_FUNC_RETURN), addr_(addr), loc_(loc), retval_(retval) {}
	~FuncReturnTransitionInfo(){}
private:
	DECL_FIELD(void*, addr)
	DECL_FIELD(SourceLocation*, loc)
	DECL_FIELD(ADDRINT, retval)
};

/********************************************************************************/

class FuncEnterTransitionInfo : public TransitionInfo {
public:
	FuncEnterTransitionInfo(void* addr, SourceLocation* loc):
		TransitionInfo(TRANS_FUNC_ENTER), addr_(addr), loc_(loc) {}
	~FuncEnterTransitionInfo(){}
private:
	DECL_FIELD(void*, addr)
	DECL_FIELD(SourceLocation*, loc)
};

/********************************************************************************/

} // namespace


#endif /* TRANSINFO_H_ */
