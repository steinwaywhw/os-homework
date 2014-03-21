#include <pthread.h>
#include "zlog.h"
#include "simple_sync.h"
#include <errno.h>

zlog_category_t *category;
simple_mutex_t mutex;
simple_cond_t cond;

void *child_timed (void *arg) {
    struct timespec ts;
    ts.tv_sec  = 1;
    ts.tv_nsec = 0;

    simple_mutex_lock (&mutex);
    
    zlog_debug (category, "Child: Wait for condition.");
    int status = simple_cond_timedwait (&cond, &mutex, &ts);
    if (status == 0)
        zlog_debug (category, "OK");
    else if (status == ETIMEDOUT)
        zlog_debug (category, "Time out");
    
    simple_mutex_unlock (&mutex);
    pthread_exit (NULL);
}

void *child_cond (void *arg) {
	simple_mutex_lock (&mutex);
	
    zlog_debug (category, "Child: Wait for condition.");
	simple_cond_wait (&cond, &mutex);
	zlog_debug (category, "OK");
	
    simple_mutex_unlock (&mutex);
    pthread_exit (NULL);
}


int main(){
	int state = zlog_init("../zlog.conf");
    category = zlog_get_category("default");
    
    pthread_t c;

    simple_mutex_init (&mutex, NULL);
    simple_cond_init (&cond, NULL);

    simple_mutex_lock (&mutex);
    int status = pthread_create (&c, NULL, child_cond, NULL);
    zlog_debug (category, "Parent: child created.");
    sleep (3);
    simple_mutex_unlock (&mutex);
    sleep (3);
    simple_cond_signal (&cond);
    sleep (3);


    // status = pthread_create (&c, NULL, child_timed, NULL);
    // zlog_debug (category, "Parent: child created.");
    // sleep (3);
    // simple_mutex_unlock (&mutex);
    // sleep (5);
    // simple_cond_signal (&cond);
    // sleep (3);


    simple_mutex_destroy (&mutex);
    simple_cond_destroy (&cond);

    zlog_fini ();
}