#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "MyScenario")

	TESTCASE() {
		CALL_TEST(SearchAll);
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh ctrace -s -c -r
	TEST(SearchAll) {
		MAX_WAIT_TIME(3*USECSPERSEC);

		TVAR(t1);
		TVAR(t2);

		FVAR(foo, foo);
		FVAR(bar, bar);

		WAIT_FOR_THREAD(t1, IN_FUNC(foo));
		WAIT_FOR_THREAD(t2, IN_FUNC(bar), ANY_THREAD - t1);

		WHILE(!HAS_ENDED(t1) || !HAS_ENDED(t2)) {

			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2));

			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || ENDS(), "Run unless");
		}
	}

	//============================================================//

	// deadlock in 54-90 executions
	// GLOG_v=1 scripts/run_bench.sh ctrace -s -c -r
	TEST(SearchLargeSteps) {
		MAX_WAIT_TIME(3*USECSPERSEC);

		TVAR(t1);
		TVAR(t2);

		FVAR(foo, foo);
		FVAR(bar, bar);

		WAIT_FOR_THREAD(t1, IN_FUNC(foo));
		WAIT_FOR_THREAD(t2, IN_FUNC(bar), ANY_THREAD - t1);

		WHILE(!HAS_ENDED(t1) || !HAS_ENDED(t2)) {

			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2));

			RUN_THREAD_THROUGH(t, HITS_PC() || ENTERS() || RETURNS() || ENDS(), "Run unless");
		}
	}

	//============================================================//

//	canonical bug trace:
//
//	//thread 0 is reading the hash table, thread 1 is not, _hashreads == 1
//
//	(0, HASH_READ_EXIT)
//	(0, pthread_mutex_lock(&_hashmutex))
//	(0, r1 = _hashreads)   // decrement is not atomic (by inspection of LLVM IR)
//	(0, r1 = r1 - 1)       // r1 == 0
//
//	(1, HASH_READ_ENTER)   // context switch
//	(1, if (!_hashreads))  // _hashreads still == 1
//	(1, r2 = _hashreads)
//	(1, r2 = r2 + 1)
//	(1, _hashreads = r2)   // _hashreads == 2
//
//	(0, _hashreads = r1)   // store thread 0's stale value into hash reads
//	(0, if(!_hashreads)
//	(0, sem_post(&_hashsem))
//	(0, pthread_mutex_unlock(&_hashmutex))
//	(0, HASH_READ_ENTER)
//	(0, if(!_hashreads))
//
//	// _hashreads is 0, so thread 0 will wait, pause 0 before it waits
//
//	(1, HASH_READ_EXIT)
//	(1, pthread_mutex_lock(&_hashmutex))
//	(1, --_hashreads)       //_hashreads == -1
//	(1, if(!_hashreads)     // if (false)
//	(1, pthread_mutex_unlock(&_hashmutex))
//	(1, thread finish)
//
//	(0, sem_wait(&_hashsem))
//	// _hashsem is now 0
//	(0, _hashreads++) //_hashreads == 0
//	(0, pthread_join)
//	(0, TRC_REMOVE_THREAD)
//	(0, int i,ret = 1)
//	(0, ...)
//	(0, HASH_WRITE_ENTER)
//	(0, sem_wait(&_hashsem))


	// GLOG_v=1 scripts/run_bench.sh ctrace -s -c -r -m1 -p1
	TEST(ExactSchedule) {
		MAX_WAIT_TIME(3*USECSPERSEC);

		FVAR(f_enter, HASH_READ_ENTER);
		FVAR(f_exit, HASH_READ_EXIT);

		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_DISTINCT_THREADS((t1, t2), IN_FUNC(f_enter));

		RUN_THREAD_UNTIL(t1, HITS_PC(43) && IN_FUNC(f_exit), "Run t1 unless 43 in f_exit");

		RUN_THREAD_UNTIL(t2, RETURNS(f_enter), "Run t2 until returns");

		RUN_THREAD_UNTIL(t1, READS() && IN_FUNC(f_enter), "Run t1 until reads in f_enter");

		RUN_THREAD_UNTIL(t2, RETURNS(f_exit), "Run t2 until returns from f_exit");

		RUN_THREAD_UNTIL(t1, ENDS(), "Run t1 until ends");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
