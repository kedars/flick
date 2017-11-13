/*! \file osal.h
 * OS Abstration Layer
 *
 */
/* All Rights Reserved */
#ifndef _FL_OSAL_H_
#define _FL_OSAL_H_

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <unistd.h>
#include <stdint.h>

/* Too many abstraction layer APIs start with os_, causing conflicts. We will
 * prefix the API with 'o'
 */
#define OS_SUCCESS 0
#define OS_FAIL    1

typedef TaskHandle_t othread_t;

#define OS_DEFAULT_PRIORITY 0

static inline int othread_create(othread_t *thread, const char *name, uint16_t stacksize, int prio,
				 void (*thread_routine)(void *arg), void *arg)
{
     int ret = xTaskCreate(thread_routine, name, stacksize, arg, prio, thread);
     if (ret == pdPASS)
          return OS_SUCCESS;
     return OS_FAIL;
}

/* Only self delete is supported */
static inline void othread_delete()
{
     vTaskDelete(xTaskGetCurrentTaskHandle());
}

static inline void othread_sleep(int msecs)
{
     vTaskDelay(msecs/portTICK_RATE_MS);
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

#endif /* ! _FL_OSAL_H_ */
