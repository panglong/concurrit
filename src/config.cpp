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

// set to default values
bool Config::OnlyShowHelp = false;
//bool Config::CanEnableDisablePinTool = true;
int Config::ExitOnFirstExecution = -1; // -1 means undefined, 0 means exit on first execution, > 0 means continue but decrease the flag (until 0)
char* Config::SaveDotGraphToFile = NULL;
//bool Config::RunUncontrolled = false;

/********************************************************************************/

static void usage() {
	printf("-h: Show this help. (OnlyShowHelp)\n"
			"-e: Enable/Disable Pin instrumentation.(CanEnableDisablePinTool)\n"
			"-fN: Exit after first N explorations. (ExitOnFirstExecution)\n"
			"-dPATH: Save dot file of the execution tree in file PATH. (SaveDotGraphToFile)\n");
}

void Config::ParseCommandLine(int argc /*= -1*/, char **argv /*= NULL*/) {
	if(argc <= 0 || argv == NULL) return;

	int c;
	opterr = 0;

	while ((c = getopt(argc, argv, "hf::d::")) != -1) {
		switch(c) {
		case 'h':
			Config::OnlyShowHelp = true;
			usage();
			break;
//		case 'e':
//			Config::CanEnableDisablePinTool = true;
//			break;
		case 'f':
			Config::ExitOnFirstExecution = (optarg == NULL) ? 1 : atoi(optarg);
			safe_assert(Config::ExitOnFirstExecution >= 1);
			printf("Will terminate the search after %d executions.\n", Config::ExitOnFirstExecution);
			break;
		case 'd':
			if(optarg != NULL) {
				Config::SaveDotGraphToFile = strdup(optarg);
			} else {
				Config::SaveDotGraphToFile = "/tmp/graph.dot";
			}
			safe_assert(Config::SaveDotGraphToFile != NULL);
			break;
//		case 'u':
//			Config::RunUncontrolled = true;
//			break;
		case '?':
			fprintf(stderr, "Unrecognized option");
			break;
		}
	}
}



} // end namespace


