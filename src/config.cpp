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
bool Config::DeleteCoveredSubtrees = true;
int Config::ExitOnFirstExecution = -1; // -1 means undefined, 0 means exit on first execution, > 0 means continue but decrease the flag (until 0)
char* Config::SaveDotGraphToFile = NULL;
long Config::MaxWaitTimeUSecs = USECSPERSEC;
bool Config::IsStarNondeterministic = false;
bool Config::RunUncontrolled = false;
char* Config::TestLibraryFile = NULL;
bool Config::KeepExecutionTree = false;
//bool Config::TrackAlternatePaths = false;
int Config::MaxTimeOutsBeforeDeadlock = 10;
bool Config::ManualInstrEnabled = true;
bool Config::PinInstrEnabled = true;
bool Config::ReloadTestLibraryOnRestart = false;
bool Config::MarkEndingBranchesCovered = true;
bool Config::SaveExecutionTraceToFile = false;
bool Config::CancelThreadsToRestart = false;
//ExecutionModeType Config::ExecutionMode = ExecutionModeType::MODE_SINGLE;

/********************************************************************************/

static void usage() {
	printf("\n=============================================\n"

			"-h: Show this help. (OnlyShowHelp)\n\n"

//			"-a: Track altenate paths (TrackAlternatePaths)\n"
			"-c[0|1]: Cut covered subtrees. (DeleteCoveredSubtrees)\n"
			"-dPATH: Save dot file of the execution tree in file PATH. (SaveDotGraphToFile)\n"
//			"-eMODE: Execution mode. MODE in [server, client]"
			"-fN: Exit after first N explorations. (ExitOnFirstExecution)\n"
			"-k: Cancel threads to restart SUT.\n"
			"-l: Test program as shared (.so) library.\n"
			"-m[0|1]: Enable/disable manual instrumentation (ManuelInstrEnabled)\n"
			"-p[0|1]: Enable pin-tool instrumentation (PinInstrEnabled)\n"
			"-r: Reload test library after each restart (ReloadTestLibraryOnRestart)\n"
			"-s[0|1]: Use stack-based DFS (!KeepExecutionTree)\n"
			"-t: Save execution trace to file (SaveExecutionTraceToFile)\n"
			"-u: Run test program uncontrolled (RunUncontrolled)\n"
			"-vN: Verbosity level (N >= 0)\n"
			"-wN: Maximum wait time (MaxWaitTimeUSecs).\n"

			"=============================================\n");
}

static bool get_bool_opt(const char* opt) {
	if(opt == NULL || strncmp(opt, "1", 1) == 0) {
		return true;
	} else if(strncmp(opt, "0", 1) == 0) {
		return false;
	}
	safe_fail("Incorrect argument, expected 0 or 1.");
}

bool Config::ParseCommandLine(const main_args& args) {
	return ParseCommandLine(args.argc_, args.argv_);
}

bool Config::ParseCommandLine(int argc /*= -1*/, char **argv /*= NULL*/) {
	if(argc <= 0 || argv == NULL) return false;

	int c;
	opterr = 0;

	while ((c = getopt(argc, argv, "c::d::f::hkl:m::p::rstuv:w:")) != -1) {
		switch(c) {
//		case 'a':
////			Config::KeepExecutionTree = true;
//			Config::TrackAlternatePaths = true;
//			Config::MarkEndingBranchesCovered = false; // alternate paths handle this
//			printf("Will track alternate paths!\n");
//			break;
		case 'c':
			Config::DeleteCoveredSubtrees = get_bool_opt(optarg);
			if(Config::DeleteCoveredSubtrees) {
				printf("Will cut covered subtrees!\n");
			} else {
				printf("Will not cut covered subtrees!\n");
			}
			break;
//		case 'e':
//			if(optarg == NULL) {
//				safe_fail("Argument of -e option is missing, server or client is required!");
//			}
//			safe_assert(Config::ExecutionMode == ExecutionModeType::MODE_SINGLE);
//			printf("Execution mode is %s\n", optarg);
//			if(strncmp(optarg, "server", 6) == 0) {
//				Config::ExecutionMode = ExecutionModeType::MODE_SERVER;
//				// we will be using our own test library file
//				safe_assert(Config::TestLibraryFile == NULL);
//				Config::TestLibraryFile = const_cast<char*>(InConcurritHomeDir("/lib/libserver.so").c_str());
//			} else if(strncmp(optarg, "client", 6) == 0) {
//				Config::ExecutionMode = ExecutionModeType::MODE_CLIENT;
//			} else {
//				safe_fail("Argument of -e option is incorrect, server or client is required!");
//			}
//			printf("Execution mode is %d.\n", Config::ExecutionMode);
//			break;
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
				Config::SaveDotGraphToFile = strdup(const_cast<char*>(InConcurritWorkDir("exec_tree.dot").c_str()));
			}
			safe_assert(Config::SaveDotGraphToFile != NULL);
			break;
		case 'k':
			Config::CancelThreadsToRestart = true;
			// killing threads may corrupt the state, so reload the library
			Config::ReloadTestLibraryOnRestart = true;
			printf("Will kill threads to restart.\n");
			break;
		case 'm':
			Config::ManualInstrEnabled = get_bool_opt(optarg);
			if(Config::ManualInstrEnabled) {
				printf("Will enable manuel instrumentation!\n");
			} else {
				printf("Will disable manuel instrumentation!\n");
			}
			break;
		case 'p':
			Config::PinInstrEnabled = get_bool_opt(optarg);
			if(Config::PinInstrEnabled) {
				printf("Will enable pin instrumentation!\n");
			} else {
				printf("Will disable pin instrumentation!\n");
			}
			break;
		case 'r':
			Config::ReloadTestLibraryOnRestart = true;
			printf("Will reload test library after each restart.\n");
			break;
		case 's':
			Config::KeepExecutionTree = !get_bool_opt(optarg);
//			Config::TrackAlternatePaths = false;
			printf("Will use stack rather than execution tree.\n");
			break;
		case 't':
			Config::SaveExecutionTraceToFile = true;
			printf("Will save execution trace to file.\n");
			break;
		case 'u':
			Config::RunUncontrolled = true;
			if(Config::ExitOnFirstExecution < 0) {
				Config::ExitOnFirstExecution = 1;
			}
			printf("Will run the test in uncontrolled mode.\n");
			break;
		case 'w':
			safe_assert(optarg != NULL);
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
		case 'v':
			safe_assert(optarg != NULL);
//			char buff[8];
//			snprintf(buff, 8, "GLOG_v=%s", optarg);
//			putenv(buff);
			setenv("GLOG_v", optarg, 1);
			safe_assert(getenv("GLOG_v") != NULL);
			printf("Verbosity level is %s.\n", getenv("GLOG_v"));
			break;
		case '?':
			fprintf(stderr, "Unrecognized option: %c\n", (char)optopt);
			break;
		}
	}

//	safe_assert(!Config::TrackAlternatePaths || Config::KeepExecutionTree);

	return true;
}



} // end namespace


