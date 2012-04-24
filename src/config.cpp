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
bool Config::DeleteCoveredSubtrees = false;
int Config::ExitOnFirstExecution = -1; // -1 means undefined, 0 means exit on first execution, > 0 means continue but decrease the flag (until 0)
char* Config::SaveDotGraphToFile = NULL;
long Config::MaxWaitTimeUSecs = 3 * USECSPERSEC;
bool Config::IsStarNondeterministic = false;
bool Config::RunUncontrolled = false;
char* Config::TestLibraryFile = NULL;
bool Config::KeepExecutionTree = true;
bool Config::TrackAlternatePaths = true;

/********************************************************************************/

static void usage() {
	printf("-h: Show this help. (OnlyShowHelp)\n"
			"-a: Track altenate paths (KeepExecutionTree && TrackAlternatePaths)"
			"-c: Cut covered subtrees. (DeleteCoveredSubtrees)\n"
			"-dPATH: Save dot file of the execution tree in file PATH. (SaveDotGraphToFile)\n"
			"-e: Enable/Disable Pin instrumentation.(CanEnableDisablePinTool)\n"
			"-fN: Exit after first N explorations. (ExitOnFirstExecution)\n"
			"-l: Test program as shared (.so) library"
			"-s: Use stack-based DFS (!KeepExecutionTree && !TrackAlternatePaths)"
			"-u: Run test program uncontrolled.");
}

bool Config::ParseCommandLine(const main_args& args) {
	return ParseCommandLine(args.argc_, args.argv_);
}

bool Config::ParseCommandLine(int argc /*= -1*/, char **argv /*= NULL*/) {
	if(argc <= 0 || argv == NULL) return false;

	int c;
	opterr = 0;

	while ((c = getopt(argc, argv, "asl:uw:chf::d::")) != -1) {
		switch(c) {
		case 'a':
			Config::KeepExecutionTree = true;
			Config::TrackAlternatePaths = true;
			printf("Will track alternate paths!\n");
			break;
		case 'c':
			Config::DeleteCoveredSubtrees = true;
			printf("Will cut covered subtrees!\n");
			break;
		case 'h':
			Config::OnlyShowHelp = true;
			usage();
			break;
		case 'f':
			Config::ExitOnFirstExecution = (optarg == NULL) ? 1 : atoi(optarg);
			safe_assert(Config::ExitOnFirstExecution >= 1);
			printf("Will terminate the search after %d executions.\n", Config::ExitOnFirstExecution);
			break;
		case 'd':
			if(optarg != NULL) {
				Config::SaveDotGraphToFile = strdup(optarg);
			} else {
				Config::SaveDotGraphToFile = strdup(const_cast<char*>(InWorkDir("exec_tree.dot")));
			}
			safe_assert(Config::SaveDotGraphToFile != NULL);
			break;
		case 's':
			Config::KeepExecutionTree = false;
			Config::TrackAlternatePaths = false;
			printf("Will use stack rather than execution tree.\n");
			break;
		case 'u':
			Config::RunUncontrolled = true;
			printf("Will run the test in uncontrolled mode.\n");
			break;
		case 'w':
			CHECK(optarg != NULL) << "Argument is missing, long value is required!";
			Config::MaxWaitTimeUSecs = atol(optarg);
			safe_assert(Config::MaxWaitTimeUSecs > 0);
			printf("MaxWaitTimeUSecs is %l.\n", Config::MaxWaitTimeUSecs);
			break;
		case 'l':
			if(optarg == NULL) {
				safe_fail("Argument of -l option is missing, a library file is required!");
			}
			Config::TestLibraryFile = strdup(optarg);
			printf("Will load the test program from the library %s.\n", Config::TestLibraryFile);
			break;
		case '?':
			fprintf(stderr, "Unrecognized option");
			break;
		}
	}

	safe_assert(!Config::TrackAlternatePaths || Config::KeepExecutionTree);

	return true;
}



} // end namespace


