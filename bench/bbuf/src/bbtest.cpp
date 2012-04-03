#include <stdio.h>

#include "boundedBuffer.h"

#define PRODUCER_SUM  1
#define CONSUMER_SUM  1


#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()


//============================================================//
//============================================================//
CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	SETUP() {
		buffer = (bounded_buf_t*) malloc(sizeof(bounded_buf_t));
		safe_assert(buffer != NULL);
		int status = bounded_buf_init(buffer, 3);
		safe_assert(status == 0);
	}

	TEARDOWN() {
		safe_assert(buffer != NULL);
		int status = bounded_buf_destroy(buffer);
		safe_assert(status == 0);
		free(buffer);
	}

	bounded_buf_t* buffer;
	thread_t producers[PRODUCER_SUM];
	thread_t consumers[CONSUMER_SUM];

	coroutine_t co_producers[PRODUCER_SUM];
	coroutine_t co_consumers[CONSUMER_SUM];

	TESTCASE() {

		TEST_FORALL();

		int i;

		for (i = 0; i < PRODUCER_SUM; i++)
		{
			producers[i].id =  i;
			producers[i].bbuf = buffer;
//			pthread_create(&producers[i].pid, NULL, producer_routine,  (void*)&producers[i]);
			co_producers[i] = CREATE_THREAD(i, producer_routine, (void*)&producers[i], true);
		}

//		printf("Created producer threads\n");

		for (i = 0; i < CONSUMER_SUM; i++)
		{
			consumers[i].id =  i;
			consumers[i].bbuf = buffer;
//			pthread_create(&consumers[i].pid, NULL, consumer_routine,  (void*)&consumers[i]);
			co_consumers[i] = CREATE_THREAD(PRODUCER_SUM+i, consumer_routine, (void*)&consumers[i], true);
		}

//		printf("Created consumer threads\n");


//		for (i = 0; i < PRODUCER_SUM; i++)
//			pthread_join(producers[i].pid, NULL);
//
//		for (i = 0; i < CONSUMER_SUM; i++)
//			pthread_join(consumers[i].pid, NULL);

//		do {
//			Thread::Yield(true);
//			short_sleep(1000);
//		} while(!ALL_ENDED);


		while(STAR)
		{
//			EXISTS(t);
//			DSLTransition(TransitionPredicate::True(), t);

//			DSLTransition(TransitionPredicate::True());

			EXISTS(t);
			DSLTransferUntil(t, TransitionPredicate::True());
		}
	}

CONCURRIT_END_TEST(BBScenario)
//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
