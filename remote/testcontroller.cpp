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

#include <cstdlib>
#include <cstdio>
#include <dlfcn.h>
#include <assert.h>

#include "concurrit.h"

#include <glog/logging.h>

/********************************************************************************/

extern "C"
int main(int argc, char* argv[]) {

	safe_check(argc == 2);
	char* cmd = argv[1];
	safe_check(cmd != NULL);

	concurrit::ConcurrentPipe* controlpipe = concurrit::ConcurrentPipe::OpenControlForSUT(NULL, false);
	concurrit::EventBuffer e;

	if(strncmp(cmd, "0", 1) == 0) {

		e.type = concurrit::TestStart;
		e.threadid = 0;
		controlpipe->Send(NULL, &e);

		fprintf(stderr, "CONTROLLER: Sent TestStart event\n");

	} else if(strncmp(cmd, "1", 1) == 0) {

		e.type = concurrit::TestEnd;
		e.threadid = 0;
		controlpipe->Send(NULL, &e);

		fprintf(stderr, "CONTROLLER: Sent TestEnd event\n");
	}

	controlpipe->Recv(NULL, &e);
	safe_assert(e.type == concurrit::Continue);

	fprintf(stderr, "CONTROLLER: Received Continue event\n");

	controlpipe->Close();

	delete(controlpipe);

	return EXIT_SUCCESS;
}

/********************************************************************************/

