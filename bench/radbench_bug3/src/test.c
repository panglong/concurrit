#include <stdlib.h>
#include <pthread.h>

#include "instrument.h"

#include "jsapi.h"

static JSRuntime *rt;

#define THREADS 3

static void * testfunc(void *ignored) {
	JSContext *cx = JS_NewContext(rt, 0x1000);
    if (cx) {
    	JS_BeginRequest(cx);
        JS_DestroyContext(cx);
    }
}

static
int main0(int argc, char* argv[]) {

    rt = JS_NewRuntime(0x100000);
    if (rt == NULL)
        return 1;

    /* Uncommenting this to guarantee there's always at least
     * one context in the runtime prevents this problem. */
//    JSContext *cx = JS_NewContext(rt, 0x1000);

    int i;
    pthread_t thread[THREADS];
    for (i = 0; i < THREADS; i++) {
        pthread_create(&thread[i], NULL, testfunc, NULL);
    }

    for (i = 0; i < THREADS; i++) {
        pthread_join(thread[i], NULL);
    }

    return 0;
}

//============================================

CONCURRIT_TEST_MAIN(main0)
