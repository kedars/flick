/*! \file osal.h
 * OS Abstration Layer
 *
 */
/* All Rights Reserved */
#ifndef _CT_OSAL_H_
#define _CT_OSAL_H_

#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

/* Too many abstraction layer APIs start with os_, causing conflicts. We will
 * prefix the API with 'o'
 */
#define OS_SUCCESS 0
#define OS_FAIL    1

typedef pthread_t othread_t;

#define OS_DEFAULT_PRIORITY 0

static inline int othread_create(othread_t *thread, const char *name, uint16_t stacksize, int prio,
				 void (*thread_routine)(void *arg), void *arg)
{
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	pthread_attr_setstacksize(&thread_attr, stacksize);

	/* For Unix, we ignore the priority and name, useful in case of the RTOS */

	return pthread_create(thread, &thread_attr, (void *(*)(void *arg)) thread_routine, arg);
}

/* Only self delete is supported */
static inline void othread_delete()
{
	pthread_exit(NULL);
}

static inline void othread_sleep(int msecs)
{
	usleep(msecs * 1000);
}


/* Memory Management */
static inline size_t os_get_current_free_mem()
{
     return 0;
}

static inline size_t os_get_minimum_free_mem()
{
     return 0;
}

#endif /* ! _CT_OSAL_H_ */
