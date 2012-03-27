#include <stdio.h>

#include "concurrit.h"
using namespace concurrit;

#include "boundedBuffer.h"

class BBScenario : public Scenario {
public:

	BBScenario() : Scenario("Bounded buffer scenario") {
		buffer = NULL;
	}
	~BBScenario() {

	}

	void SetUp() {
		buffer = (bounded_buf_t*) malloc(sizeof(bounded_buf_t));
		safe_assert(buffer != NULL);
		int status = bounded_buf_init(buffer, 3);
		safe_assert(status == 0);
	}

	void TearDown() {
		safe_assert(buffer != NULL);
		int status = bounded_buf_destroy(buffer);
		safe_assert(status == 0);
		free(buffer);
	}

	bounded_buf_t* buffer;
	thread_t producers[PRODUCER_SUM];
	thread_t consumers[CONSUMER_SUM];

	void TestCase() {

		TEST_FORALL();

		int i;

		for (i = 0; i < PRODUCER_SUM; i++)
		{
			producers[i].id =  i;
			producers[i].bbuf = buffer;
//			pthread_create(&producers[i].pid, NULL, producer_routine,  (void*)&producers[i]);
			coroutine_t co = CREATE_THREAD(i, producer_routine, (void*)&producers[i], true);
		}

//		printf("Created producer threads\n");

		for (i = 0; i < CONSUMER_SUM; i++)
		{
			consumers[i].id =  i;
			consumers[i].bbuf = buffer;
//			pthread_create(&consumers[i].pid, NULL, consumer_routine,  (void*)&consumers[i]);
			coroutine_t co = CREATE_THREAD(PRODUCER_SUM+i, consumer_routine, (void*)&consumers[i], true);
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


//		int k = 1;
//		while(STAR) {
//			printf("Iteration %d\n", k);
//			++k;
//
//			if(k == 5) {
//				break;
////				ASSERT(false);
//			}
//		}

//		int k = 1;
//		while(STAR) {
//			DSLTransition(TransitionPredicate::True());
//			k++;
//		}
//		printf("Ending with k=%d\n", k);


		while(STAR)
		{
			EXISTS(t);
			DSLTransition(TransitionPredicate::True());
		}

//		{
//					EXISTS(t);
//					DSLTransition(TransitionPredicate::True());
//				}
//
//		{
//					EXISTS(t);
//					DSLTransition(TransitionPredicate::True());
//				}
	}

};


int main(int argc, char ** argv)
{
//  thread_t producers[PRODUCER_SUM];
//  thread_t consumers[CONSUMER_SUM];
//  int i;

	INIT_CONCURRIT(argc, argv);

	Suite suite;

	suite.AddScenario(new BBScenario());

	suite.RunAll();

//  bounded_buf_t buffer;
//  bounded_buf_init(&buffer, 3);
//
//  for (i = 0; i < PRODUCER_SUM; i++)
//  {
//    producers[i].id =  i;
//    producers[i].bbuf = &buffer;
//    pthread_create(&producers[i].pid, NULL, producer_routine,  (void*)&producers[i]);
//  }
//
//  for (i = 0; i < CONSUMER_SUM; i++)
//  {
//    consumers[i].id =  i;
//    consumers[i].bbuf = &buffer;
//    pthread_create(&consumers[i].pid, NULL, consumer_routine,  (void*)&consumers[i]);
//  }
//
//
//  for (i = 0; i < PRODUCER_SUM; i++)
//    pthread_join(producers[i].pid, NULL);
//
//  for (i = 0; i < CONSUMER_SUM; i++)
//    pthread_join(consumers[i].pid, NULL);
//
//  bounded_buf_destroy(&buffer);

  return 0;
}

