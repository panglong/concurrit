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


#ifndef VC_H_
#define VC_H_

#include "common.h"

namespace concurrit {

typedef std::map<THREADID,vctime_t> VC;
typedef std::map<ADDRINT,VC> VCMAP;

extern VC vc_get_vc(VCMAP& m, ADDRINT k);
extern void vc_set_vc(VCMAP& m, ADDRINT k, VC vc);

extern void vc_clear(VC& v);
extern vctime_t vc_get(VC& vc, THREADID t);
extern void vc_set(VC& vc, THREADID t, vctime_t c);

extern void vc_inc(VC& vc, THREADID t);
extern VC vc_cup(VC& vc1, VC& vc2);

extern bool vc_leq(VC& vc1, VC& vc2, THREADID t);
extern bool vc_leq_all(VC& vc1, VC& vc2);

} // end namespace

#endif // VC_H_
