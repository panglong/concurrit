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

#ifndef COUNIT_H_
#define COUNIT_H_

#include "common.h"
#include "thread.h"

/********************************************************************************/

namespace concurrit {
	extern std::string CONCURRIT_HOME;

	class Scenario;
	class Semaphore;

	class Concurrit {
	public:
		static void Init(int argc = -1, char **argv = NULL);
		static void Destroy();
		static volatile bool IsInitialized();
		static void* CallDriverMain(void*);
		static void LoadTestLibrary();
		static void UnloadTestLibrary();

		static void InstallSignalHandler();
		static void SignalHandler(int signal_number);
	private:
		static volatile bool initialized_;
		DECL_STATIC_FIELD_REF(main_args, driver_args)
		DECL_STATIC_FIELD(void*, driver_handle)
		DECL_STATIC_FIELD(MainFuncType, driver_main)
	//	DECL_STATIC_FIELD(MainFuncType, driver_init)
	//	DECL_STATIC_FIELD(MainFuncType, driver_fini)
		DECL_STATIC_FIELD_REF(Semaphore, sem_driver_load)
		DECL_STATIC_FIELD_REF(Semaphore, sem_test_start)
		DECL_STATIC_FIELD(Scenario*, current_scenario)
	};
} // end namespace

/********************************************************************************/

#define InConcurritHomeDir(f)	(CONCURRIT_HOME + "/" + f)
#define InConcurritWorkDir(f)	(CONCURRIT_HOME + "/work/" + f)

/********************************************************************************/

// google libraries
#include <glog/logging.h>

// boost libraries
#include <boost/shared_ptr.hpp>

// Mersenne Twister random number generator
#include "MersenneTwister.h"

// GNU portable threads library
// #include "pth.h"

// pthreads library
#include <pthread.h>

/********************************************************************************/

#include "util.h"
#include "config.h"
#include "str.h"
#include "iterator.h"
#include "statistics.h"
#include "serialize.h"
#include "sharedaccess.h"
#include "schedule.h"
#include "api.h"
#include "thread.h"
#include "channel.h"
#include "coroutine.h"
#include "group.h"
#include "exception.h"
#include "result.h"
#include "scenario.h"
#include "suite.h"
#include "dot.h"
#include "interface.h"
#include "pinmonitor.h"
#include "predicate.h"
#include "dsl.h"
#include "transpred.h"
#include "instrument.h"
#include "manual.h"
#include "interpos.h"
#include "default.h"
#include "ipc.h"

/********************************************************************************/

#endif /* COUNIT_H_ */
