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

std::string CONCURRIT_HOME;

volatile bool Concurrit::initialized_ = false;
main_args Concurrit::driver_args_;
void* Concurrit::driver_handle_ = NULL;
MainFuncType Concurrit::driver_main_ = NULL;
//MainFuncType Concurrit::driver_init_ = NULL;
//MainFuncType Concurrit::driver_fini_ = NULL;
Semaphore Concurrit::sem_driver_load_;
Semaphore Concurrit::sem_test_start_;
Scenario* Concurrit::current_scenario_ = NULL;

/********************************************************************************/

extern "C"
__attribute__((constructor))
void Concurrit_Constructor() {

	// init logging
	google::InitGoogleLogging("concurrit");

	PthreadOriginals::initialize();
}

/********************************************************************************/

extern "C"
__attribute__((destructor))
void Concurrit_Destructor() {

	// finalize google logging
	google::ShutdownGoogleLogging();
}

/********************************************************************************/

void Concurrit::Init(int argc /*= -1*/, char **argv /*= NULL*/) {
	safe_assert(!IsInitialized());

	//==========================================

	// init work dir
	CONCURRIT_HOME = std::string(safe_notnull(getenv("CONCURRIT_HOME")));

	//==========================================

	Concurrit::InstallSignalHandler();

	//==========================================
	// separate arguments into two: 1 -- 2

	fprintf(stderr, "Test arguments: %s\n", main_args(argc, argv).ToString().c_str());

	safe_assert(argc > 0);
	std::vector<char*> args;
	int i = 0;
	// 1
	for(; i < argc; ++i) {
		if(strncmp(safe_notnull(argv[i]), "--", 2) == 0) break;
		args.push_back(argv[i]);
	}

	main_args m = ArgVectorToMainArgs(args);
	fprintf(stderr, "Concurrit arguments: %s\n", m.ToString().c_str());

	Config::ParseCommandLine(m.argc_, m.argv_);
	if(Config::OnlyShowHelp) {
		safe_exit(EXIT_SUCCESS);
	}

	// 2
	if(i < argc && strncmp(argv[i], "--", 2) == 0) {
		++i;
	}
	args.clear();
	args.push_back("<bench-name>");
	for(; i < argc; ++i) {
		args.push_back(argv[i]);
	}
	Concurrit::driver_args_ = ArgVectorToMainArgs(args);
	fprintf(stderr, "Driver arguments: %s\n", Concurrit::driver_args_ .ToString().c_str());

	//==========================================

	if(Config::RunUncontrolled || !Config::PinInstrEnabled) {
		PinMonitor::Shutdown();
	}

	//==========================================

	Concurrit::sem_driver_load_.Init(0);

	Concurrit::sem_test_start_.Init(0);

	// init test library. the library is to be loaded by RunTestDriver
	Concurrit::driver_handle_ = NULL;
	Concurrit::driver_main_ = NULL;
//	Concurrit::driver_init_ = NULL;
//	Concurrit::driver_fini_ = NULL;

	//==========================================

// init pth
//	int pth_init_result = pth_init();
//	safe_assert(pth_init_result == TRUE);

	AuxState::Init();

	PthreadHandler::Current = new ConcurritPthreadHandler();

	InstrHandler::Current = new ConcurritInstrHandler();

	Thread::init_tls_key();

	Coroutine::StartMain();

	PinMonitor::Init();

	do { // need a fence here
		initialized_ = true;
	} while(false);

}

/********************************************************************************/

void Concurrit::Destroy() {
	safe_assert(IsInitialized());

	safe_assert(!PinMonitor::IsEnabled());
	PinMonitor::Shutdown();

	do { // need a fence here
		initialized_ = false;
	} while(false);

	safe_assert(INSTANCEOF(PthreadHandler::Current, ConcurritPthreadHandler*));
	safe_delete(PthreadHandler::Current);

	safe_assert(INSTANCEOF(InstrHandler::Current, ConcurritInstrHandler*));
	safe_delete(InstrHandler::Current);

	Coroutine::FinishMain();

	Thread::delete_tls_key();

//	int pth_kill_result = pth_kill();
//	safe_assert(pth_kill_result == TRUE);
}

/********************************************************************************/

volatile bool Concurrit::IsInitialized() {
	return initialized_;
}

/********************************************************************************/

void Concurrit::LoadTestLibrary() {
	safe_assert(Concurrit::driver_handle_ == NULL);
	void* handle = NULL;
	if(Config::TestLibraryFile != NULL) {
		handle = dlopen(Config::TestLibraryFile, RTLD_LAZY | RTLD_LOCAL);
		if(handle == NULL) {
			safe_fail("Cannot load the test library %s!\n", Config::TestLibraryFile);
		}
		Concurrit::driver_handle_ = handle;
		Concurrit::driver_main_ = (MainFuncType) FuncAddressByName("__main__", handle, true);
//		Concurrit::driver_init_ = (MainFuncType) FuncAddressByName("__init__", handle, false);
//		Concurrit::driver_fini_ = (MainFuncType) FuncAddressByName("__fini__", handle, false);
		MYLOG(1) << "Loaded the test library " << Config::TestLibraryFile;
		dlerror(); // Clear any existing error
	} else {
		Concurrit::driver_handle_ = NULL;
		Concurrit::driver_main_ = NULL;
//		Concurrit::driver_init_ = NULL;
//		Concurrit::driver_fini_ = NULL;
		MYLOG(1) << "Not loading a test library";
	}
}

/********************************************************************************/

void Concurrit::UnloadTestLibrary() {
	safe_assert(Concurrit::driver_handle_ != NULL);

	if(dlclose(Concurrit::driver_handle_)) {
		safe_fail("Cannot unload the test library %s!\n", Config::TestLibraryFile);
	}
	dlerror(); // Clear any existing error

	Concurrit::driver_handle_ = NULL;
	Concurrit::driver_main_ = NULL;
//	Concurrit::driver_init_ = NULL;
//	Concurrit::driver_fini_ = NULL;
}

/********************************************************************************/

// run by a new thread created by the script before calling TestCase
void* Concurrit::CallDriverMain(void*) {
	// try to unload and load the driver
	void* handle = Concurrit::driver_handle();
	if(Config::ReloadTestLibraryOnRestart && handle != NULL) {
		Concurrit::UnloadTestLibrary();
		handle = NULL;
	}
	if(handle == NULL) {
		Concurrit::LoadTestLibrary();
	}
	Concurrit::sem_driver_load()->Signal();

	//------------------------------------------------

	MainFuncType main_func = Concurrit::driver_main();
	safe_assert(main_func != NULL);
	if(main_func == NULL) {
		safe_fail("Default __main__ implementation should not be called!");
	}

	safe_assert(driver_args_.check());

//	MainFuncType init_func = Concurrit::driver_init();
//	if(init_func != NULL) {
//		MYLOG(1) << "Calling driver __init__ function.";
//		init_func(driver_args_.argc_, driver_args_.argv_);
//		Concurrit::set_driver_init(NULL); // since we are calling it once
//	}

	// call the driver
	main_func(driver_args_.argc_, driver_args_.argv_);
}

/********************************************************************************/

const int signals_handled[] = { SIGSEGV, SIGABRT, SIGTERM, SIGTSTP, SIGINT };

void Concurrit::InstallSignalHandler() {
	struct sigaction sig_action;
	memset(&sig_action, 0, sizeof(sig_action));
	sigemptyset(&sig_action.sa_mask);
//	sig_action.sa_flags |= SA_RESTART;
	sig_action.sa_handler = &Concurrit::SignalHandler;

	for (size_t i = 0; i < 5; ++i) {
		CHECK_ERR(sigaction(signals_handled[i], &sig_action, NULL));
	}
}

static std::atomic<pthread_t> signal_handling_thread(PTH_INVALID_THREAD);

void Concurrit::SignalHandler(int signal_number) {
	pthread_t empty_thread = PTH_INVALID_THREAD;
	pthread_t self_thread = pthread_self();
	if(signal_handling_thread.compare_exchange_strong(empty_thread, self_thread)) {
		// first disable pin instrumentation
		PinMonitor::Disable();
		PinMonitor::Shutdown();

		// then finish the current scenario, if exists
		Scenario* scenario = Scenario::Current();
		if(scenario != NULL) {
			scenario->OnSignal(signal_number);
		}

		// finally, let google::logging do rest of the work
		google::InstallFailureSignalHandler();
		kill(getpid(), signal_number);

	} else if(signal_handling_thread.load() == self_thread) {
		fprintf(stderr, "Triggered signal while handling another one!\n");
		fflush(stderr);

		// note: SIGKILL is not handled, thus breaks the recursion
		print_stack_trace();
		kill(getpid(), SIGKILL);
	}
}

/********************************************************************************/

} // end namespace
