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

/********************************************************************************/

const char* CONCURRIT_HOME = NULL;

volatile bool Concurrit::initialized_ = false;
main_args Concurrit::driver_args_;
MainFuncType Concurrit::driver_main_;

void Concurrit::Init(int argc /*= -1*/, char **argv /*= NULL*/) {
	safe_assert(!IsInitialized());

	//==========================================

	// first set the signal handler
	struct sigaction sigact;

	 sigact.sa_sigaction = Concurrit::SignalHandler;
	 sigact.sa_flags = SA_RESTART | SA_SIGINFO;

	 if (sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL) != 0)
	 {
	  fprintf(stderr, "error setting signal handler for %d (%s)\n",
	    SIGSEGV, strsignal(SIGSEGV));

	  exit(EXIT_FAILURE);
	 }


	//==========================================
	// separate arguments into two: 1 -- 2

	fprintf(stderr, "Test arguments: %s\n", main_args(argc, argv).ToString().c_str());

	safe_assert(argc > 0);
	Concurrit::driver_args_ = {0, NULL};
	if(argc > 1) {
		std::vector<char*> args;
		int i = 1;
		// 1
		for(; i < argc; ++i) {
			if(strncmp(argv[i], "--", 2) == 0) break;
			args.push_back(argv[i]);
		}

		main_args m = ArgVectorToMainArgs(args);
		fprintf(stderr, "Concurrit arguments: %s\n", m.ToString().c_str());

		Config::ParseCommandLine(m.argc_, m.argv_);
		if(Config::OnlyShowHelp) {
			_Exit(EXIT_SUCCESS);
		}

		// 2
		if(strncmp(argv[i], "--", 2) == 0) {
			++i;
		}
		if(i < argc) {
			args.clear();
			for(; i < argc; ++i) {
				args.push_back(argv[i]);
			}
			Concurrit::driver_args_ = ArgVectorToMainArgs(args);
		}
	}
	fprintf(stderr, "Driver arguments: %s\n", Concurrit::driver_args_ .ToString().c_str());

	//==========================================

	// find the driver main function
	Concurrit::driver_main_ = (MainFuncType) dlsym(RTLD_NEXT, "__main__");
	if(Concurrit::driver_main_ == NULL) { \
		fprintf(stderr, "Concurrit: RTLD_NEXT init of __main__ failed, using the RTLD_DEFAULT init.\n");
		Concurrit::driver_main_ = (MainFuncType) dlsym(RTLD_DEFAULT, "__main__");
	}
	CHECK(Concurrit::driver_main_ != NULL) << "Concurrit: __main__ init failed!";

	//==========================================

// init pth
//	int pth_init_result = pth_init();
//	safe_assert(pth_init_result == TRUE);

	// init work dir
	CONCURRIT_HOME = getenv("CONCURRIT_HOME");

	// init logging
	google::InitGoogleLogging("concurrit");

	Originals::initialize();

	Thread::init_tls_key();

	CoroutineGroup::init_main();

	PinMonitor::Init();

	do { // need a fence here
		initialized_ = true;
	} while(false);

	VLOG(2) << "Initialized Concurrit.";
}

void Concurrit::Destroy() {
	safe_assert(IsInitialized());

	CoroutineGroup::delete_main();

	Thread::delete_tls_key();

//	int pth_kill_result = pth_kill();
//	safe_assert(pth_kill_result == TRUE);

	do { // need a fence here
		initialized_ = false;
	} while(false);

	VLOG(2) << "Finalized Concurrit.";
}

/********************************************************************************/

volatile bool Concurrit::IsInitialized() {
	return initialized_;
}

/********************************************************************************/

//============================================

// code to add to benchmarks
/*

#ifdef __cplusplus
extern "C" {
#endif
int __main__(int argc, char* argv[]) {
	return main(argc, argv);
}
#ifdef __cplusplus
} // extern "C"
#endif

*/
//============================================

// default, empty implementation of test driver
int __main__ (int, char**) {
	VLOG(2) << "Running default test-driver main function.";
}

void* Concurrit::CallDriverMain(void*) {
	MainFuncType main_func = Concurrit::driver_main();
	safe_assert(main_func != NULL);
	// call the driver
	main_func(driver_args_.argc_, driver_args_.argv_);
}

/********************************************************************************/

void Concurrit::SignalHandler(int sig_num, siginfo_t * info, void * ucontext) {
	fprintf(stderr, "Got signal %s!\n", strsignal(sig_num));
	print_stack_trace();
	fflush(stderr);
	_Exit(UNRECOVERABLE_ERROR);
}

} // end namespace
