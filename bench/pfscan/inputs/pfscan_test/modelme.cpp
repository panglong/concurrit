/**
* libmodel - Pthreads model checking library 
* 
* Author - Nick Jalbert (jalbert@eecs.berkeley.edu) 
*
* <Legal matter>
*/

#ifndef _LIBMODEL_H_
#define _LIBMODEL_H_

#include "modeltypes.h"
#include "libpth.h"
#include "tlsmodel.h"

ofstream schedfile;
ofstream choicefile;
ofstream debugfile;
ifstream fin;
ofstream dsfile;
ofstream dbgprfile;


class ModelHandler : public Handler {
    public:
        ModelHandler() : Handler() {
            dbgPrint("Starting Modelchecking...\n");
        }

        virtual ~ModelHandler() {
            schedfile.close();
            choicefile.close();
            debugfile.close();
            dsfile.close();
            dbgprfile.close();
            debugPrint(0, "Ending Modelchecking...\n");
        }

        void debugPrint(int tid, string message) {
            if (DEBUG) {
                cout<<"\tDthreads: "<<tid<<": "<<message<<endl<<flush;
                dbgprfile <<"\tDthreads: "<<tid<<": "<<message<<endl<<flush;
            }
        }


        friend void myMemoryRead(int, void*);
        friend void myMemoryWrite(int, void*);

    protected:	
        virtual void AfterInitialize();
        virtual void ThreadStart(void * arg);
        virtual void ThreadFinish(void * status);
        virtual bool BeforeCreate(pthread_t* newthread, 
                const pthread_attr_t *attr,
                void *(*start_routine) (void *), 
                void *arg, ThreadInfo* &tInfo);
        virtual void AfterCreate(int& ret_val, 
                pthread_t* newthread, 
                const pthread_attr_t *attr,
                void *(*start_routine) (void *), 
                void *arg);
        virtual bool BeforeJoin(pthread_t th, void** thread_return);
        virtual bool BeforeMutexLock(pthread_mutex_t *mutex);
        virtual void AfterMutexUnlock(int& ret_val, pthread_mutex_t *mutex);
        virtual bool BeforeCondWait(pthread_cond_t *cond, 
                pthread_mutex_t *mutex);
        virtual int SimulateCondWait(pthread_cond_t *cond, 
                pthread_mutex_t *mutex);
        virtual bool BeforeCondSignal(pthread_cond_t * cond);
        virtual int SimulateCondSignal(pthread_cond_t *cond);
        virtual bool BeforeCondBroadcast(pthread_cond_t * cond); 
        virtual int SimulateCondBroadcast(pthread_cond_t *cond);
        virtual void myMemoryRead(int iid, void * addr);
        virtual void myMemoryWrite(int iid, void * addr);
        virtual void setTID(pthread_t);
        virtual int getParticularTID(pthread_t mythr);
        virtual int getTID();
        virtual void enableThread(int id);
        virtual bool isEnabled(int id);
        virtual void disableThread(int id);
        virtual void pauseThread(int id, string * status);
        virtual void wakeThread(int id);
        virtual void waitPauseThread(int id, string * status);
        virtual void waitWakeThread(int id);
        virtual void finishThread(int id);
        virtual int prepareThread();
        virtual void recordChoice(int chosenThread);
        virtual void recordSignalChoice(int chosenThread, 
                bool waiting_cond_threads[], 
                int waiting_threads_length);
        virtual void dumpState(int choicefail); 
        virtual int getRandomEnabled();
        virtual int getNextSchedulingChoice(); 
        virtual void deadlockError(); 
        virtual void scheduleError(int nextchoice); 
        virtual bool schedEnable(int nextchoice); 
        virtual void normalSchedule(int nextchoice); 
        virtual void schedule(); 
        virtual void scheduleDebug(int tid, string s); 

    private:
        string str_threadstart;
        string str_beforejoin;
        string str_aftercreate;
        string str_beforelock;
        string str_aftersignal;
        string str_lostsignal;
        string str_inwait;
        string str_notwaiting;
        string str_afterunlock;
        string str_afterbroadcast;
        string str_lostbroadcast;
        string rf_dl; 

        bool broadcast_time[MAXTHREADS];
        volatile Boolwrapper * pthread_exit_map[MAXTHREADS];
        volatile Boolwrapper * finish_map[MAXTHREADS];
        sem_t * sem_map[MAXTHREADS];
        sem_t * wait_sem_map[MAXTHREADS];
        volatile Boolwrapper * enable_map[MAXTHREADS];
        volatile pthread_mutex_t * ready_array[MAXTHREADS];
        volatile pthread_mutex_t * wait_array[MAXTHREADS];
        volatile pthread_t * thr_map[MAXTHREADS];
        volatile pthread_t id_map[MAXTHREADS];
        volatile pthread_cond_t * cond_map[MAXTHREADS];
        volatile Boolwrapper * rfAccessIsRead[MAXTHREADS];
        volatile Boolwrapper * rfWaiters[MAXTHREADS];
        volatile void * rfAccessAddr[MAXTHREADS];
        volatile int join_map[MAXTHREADS];
        volatile string * status_array[MAXTHREADS];
        volatile int thread_id;
        volatile int total_threads;
        volatile bool first_create;
        volatile int private_counter;
        volatile bool is_wait;
        pthread_mutex_t serializer;
        pthread_cond_t serial_cond;


};


#endif
