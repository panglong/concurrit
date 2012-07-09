#include <stdio.h>
#include <stdlib.h>

#include "interface.h"
#include "ipc.h"

unsigned int g_loop = 10;
unsigned int g_threads = 10;

void* worker(void*) {

//	for(int i = 0; i < 10; ++i) {
		system("wget http://localhost:8090/ -o /dev/null -O /dev/null");
//	}

	return NULL;
}

int main(int argc, char* argv[])
{
	fprintf(stderr, "HARNESS: Openning pipe\n");

	concurrit::ConcurrentPipe* controlpipe = concurrit::ConcurrentPipe::OpenControlForSUT(NULL, false);

	fprintf(stderr, "HARNESS: Starting\n");

	//====================================================
	//====================================================

	for(;;) {

		fprintf(stderr, "HARNESS: Sending TestStart event\n");

		concurrit::EventBuffer e;
		e.type = concurrit::TestStart;
		e.threadid = 0;
		controlpipe->Send(NULL, &e);

		controlpipe->Recv(NULL, &e);
		safe_assert(e.type == Continue);

		fprintf(stderr, "HARNESS: Sent TestStart event\n");

		//====================================================
		//====================================================

		{
			int i;
			pthread_t threads[g_threads];

			for(i=0; i < g_threads; ++i) {
				int err = pthread_create(&threads[i], NULL, worker, NULL);
				if(err != 0) {
					fprintf(stderr, "failed to create thread: %s\n", strerror(err));
					exit(1);
				}
			}

			for(i=0; i < g_threads; ++i) {
				void* ret;
				fprintf(stderr, "HARNESS: Joining thread %d\n", i);
				int err = pthread_join(threads[i], &ret);
				if(err != 0) {
					fprintf(stderr, "failed to join thread: %s\n", strerror(err));
				}
			}
		}

		fprintf(stderr, "HARNESS: Joined threads\n");

		//====================================================
		//====================================================

		e.type = concurrit::TestEnd;
		e.threadid = 0;
		controlpipe->Send(NULL, &e);

		controlpipe->Recv(NULL, &e);
		safe_assert(e.type == Continue);

		fprintf(stderr, "HARNESS: Sent TestEnd event\n");

	} // end for

	controlpipe->Close();

	delete(controlpipe);

	fprintf(stderr, "HARNESS: Exiting\n");

	return 0;
}
