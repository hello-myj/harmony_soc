#include "ezos.h"

#if 1
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#if 0
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "list.h"
#include "queue.h"
#include "semphr.h"
#else

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/list.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"



#endif

#define DONT_BLOCK 0
#define VALUE_BEFORE_TIMER_START 1

extern BaseType_t xPortCheckIfInISR(void);

static inline uint32_t ezos_irq_context(void)
{
    uint32_t irq;
    BaseType_t state;

    irq = 0U;
    /* Get FreeRTOS scheduler state */
    state = xTaskGetSchedulerState();

    if (state != taskSCHEDULER_NOT_STARTED)
    {
        /* Scheduler was started */
        if (xPortCheckIfInISR())
        {
            /* Interrupts are masked */
            irq = 1U;
        }
    }
    /* Return context, 0: thread context, 1: IRQ context */
    return (irq);
}

//  ==== Thread Functions ====
ezos_thread_id_t ezos_thread_create(ezos_thread_func_cb func, ezos_thread_params_t *param)
{
    TaskHandle_t task_handle = NULL;
    ezos_thread_params_t tmp_param = {0};
    uint32_t prio;
    if (func == NULL)
        return NULL;

    if (param == NULL)
    {
        tmp_param.user_arg = NULL;
        tmp_param.priority = 16;
        tmp_param.thread_name = NULL;
        tmp_param.stack_size = configMINIMAL_STACK_SIZE;
        param = &tmp_param;
    }

    if (param->priority > EZ_MAX_PRIORITY)
        return NULL;

    prio = ((float)configMAX_PRIORITIES / EZ_MAX_PRIORITY) * param->priority;

    int ret = xTaskCreate((TaskFunction_t)func, param->thread_name, param->stack_size, param->user_arg, prio, &task_handle);
    if (ret != pdPASS)
    {
        return NULL;
    }
    return task_handle;
}

void ezos_thread_destroy(ezos_thread_id_t id)
{
    vTaskDelete((TaskHandle_t)id);
}

ezos_status_t ezos_thread_suspend(ezos_thread_id_t id)
{
    if (id == NULL)
        return EZOS_EINVAL;

    if (ezos_irq_context() != 0)
    {
        return EZOS_ERRISR;
    }

    vTaskSuspend((TaskHandle_t)id);
    return EZOS_SUCCESS;
}

ezos_status_t ezos_thread_resume(ezos_thread_id_t id)
{

    if (id == NULL)
        return EZOS_EINVAL;

    if (ezos_irq_context() != 0)
    {
        xTaskResumeFromISR((TaskHandle_t)id);
    }
    else
    {
        vTaskResume((TaskHandle_t)id);
    }

    return EZOS_SUCCESS;
}

ezos_status_t ezos_thread_yield(void)
{
    taskYIELD();
    return EZOS_SUCCESS;
}

//  ==== Memory Functions ====
void *ezos_malloc(uint32_t size)
{

    return pvPortMalloc(size);
}

void *ezos_realloc(void *ptr, uint32_t size)
{
    return realloc(ptr, size);
}

void *ezos_calloc(uint32_t nitems, size_t size)
{
    return calloc(nitems, size);
}

void ezos_free(void *ptr)
{
    vPortFree(ptr);
}

//  ==== mutex Functions ====
/// @brief 创建锁
/// @return 返回NULL，则失败
ezos_mutex_id_t ezos_mutex_create(void)
{
#if (configUSE_MUTEXES == 1)
    return xSemaphoreCreateMutex();
#endif
}

/// @brief 删除锁
/// @param mutex 锁句柄
ezos_status_t ezos_mutex_destroy(ezos_mutex_id_t mutex)
{
    if (mutex == NULL)
    {
        return EZOS_EINVAL;
    }
    vSemaphoreDelete(mutex);
    return 0;
}

/// @brief 锁住
/// @param mutex 锁句柄
ezos_status_t ezos_mutex_lock(ezos_mutex_id_t mutex)
{
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) // 获取失败阻塞
    {
        return (EZOS_SUCCESS);
    }
    return (EZOS_FAILURE);
}

/// @brief 解锁
/// @param mutex 锁句柄
ezos_status_t ezos_mutex_unlock(ezos_mutex_id_t mutex)
{
    if (xSemaphoreGive(mutex) == pdTRUE)
    {
        return (EZOS_SUCCESS);
    }
    return (EZOS_FAILURE);
}

//  ==== Semaphore Management Function ====
/// @brief 创建信号量
/// @param max_count   信号量最大计数器
/// @param initial_count    信号量计数器初始值
/// @return 成功，返回一个信号量，失败返回NULL
ezos_sem_id_t ezos_sem_create(uint32_t max_count, uint32_t initial_count)
{
    if (max_count <= 0 )
    {
        return NULL;
    }

    if (max_count < initial_count)
    {
        return NULL;
    }

    return xSemaphoreCreateCounting(max_count, initial_count);
}

/// @brief 删除信号量
/// @param sem 信号量
ezos_status_t ezos_sem_destroy(ezos_sem_id_t sem)
{
    if (sem == NULL)
    {
        return EZOS_EINVAL;
    }
    vSemaphoreDelete(sem);
    return EZOS_SUCCESS;
}

/// @brief 等待获取信号量，获取不到，当前阻塞
/// @param sem  信号量
/// @param timeout 超时时间。传入0表示不超时，立即返回；0xFFFFFFFFFF表示永久等待;其他数值表示超时时间，单位ms。
/// @return    - 0: 成功，-1：失败，其他：返回超时
ezos_status_t ezos_sem_take(ezos_sem_id_t sem, uint32_t timeout)
{
    int ret;

    if (!sem)
    {
        return EZOS_EINVAL;
    }

    if (ezos_irq_context())
    { // 判断是否处于中断
        ret = xSemaphoreTakeFromISR(sem, NULL);
    }
    else
    {
        ret = xSemaphoreTake(sem, timeout);
    }
    return ret ? EZOS_SUCCESS : EZOS_EBUZY;
}

/// @brief 释放信号量
/// @param sem 信号量
ezos_status_t ezos_sem_give(ezos_sem_id_t sem)
{
    int ret = 0;
    if (sem == NULL)
    {
        return EZOS_EINVAL;
    }

    if (ezos_irq_context())
    { // 判断是否处于中断
        ret = xSemaphoreGiveFromISR(sem, NULL);
    }
    else
    {
        ret = xSemaphoreGive(sem);
    }
    return ret ? EZOS_SUCCESS : EZOS_EBUZY;
}

//  ==== Message Queue Management Functions====
ezos_queue_id_t ezos_queue_create(uint32_t msg_count, uint32_t msg_size)
{
    if (msg_count == 0 || msg_count == 0)
        return NULL;
    QueueHandle_t queue_id = NULL;
    queue_id = xQueueCreate(msg_count, msg_size);
    return queue_id;
}

void ezos_queue_destroy(ezos_queue_id_t queue)
{
    vQueueDelete((QueueHandle_t)queue);
}

ezos_status_t ezos_queue_write(ezos_queue_id_t queue, void *msg_ptr, uint32_t msg_size, uint32_t timeout)
{
    ezos_status_t ret = EZOS_SUCCESS;
    QueueHandle_t queue_id = (QueueHandle_t)queue;

    if (queue_id == NULL || msg_ptr == NULL)
        return EZOS_EINVAL;

    if (ezos_irq_context() != 0)
    {
        BaseType_t yield = pdFALSE;
        if (xQueueSendFromISR(queue_id, msg_ptr, &yield) != pdTRUE)
        {
            ret = EZOS_FAILURE;
        }
        else
        {
            portYIELD_FROM_ISR(yield);
        }
    }
    else
    {
        if (xQueueSend(queue_id, msg_ptr, (TickType_t)timeout) != pdTRUE)
        {
            ret = EZOS_FAILURE;
            if (timeout != 0)
            {
                ret = EZOS_TIMEOUT;
            }
        }
    }
    return ret;
}

ezos_status_t ezos_queue_read(ezos_queue_id_t queue, void *msg_ptr, uint32_t msg_size, uint32_t timeout)
{
    ezos_status_t ret = EZOS_SUCCESS;
    QueueHandle_t queue_id = (QueueHandle_t)queue;

    if (queue_id == NULL || msg_ptr == NULL)
        return EZOS_EINVAL;

    if (ezos_irq_context() != 0)
    {
        BaseType_t yield = pdFALSE;
        if (xQueueReceiveFromISR(queue_id, msg_ptr, &yield) != pdTRUE)
        {
            ret = EZOS_FAILURE;
        }
        else
        {
            portYIELD_FROM_ISR(yield);
        }
    }
    else
    {
        if (xQueueReceive(queue_id, msg_ptr, (TickType_t)timeout) != pdTRUE)
        {
            ret = EZOS_FAILURE;
            if (timeout != 0)
            {
                ret = EZOS_TIMEOUT;
            }
        }
    }
    return ret;
}

uint32_t ezos_queue_count_get(ezos_queue_id_t queue)
{
    QueueHandle_t queue_id = (QueueHandle_t)queue;
    UBaseType_t count;

    if (queue_id == NULL)
    {
        return 0;
    }

    if (ezos_irq_context() != 0U)
    {
        count = uxQueueMessagesWaitingFromISR(queue_id);
    }
    else
    {
        count = uxQueueMessagesWaiting(queue_id);
    }

    return (uint32_t)count;
}

void ezos_queue_reset(ezos_queue_id_t queue)
{
    (void)xQueueReset((QueueHandle_t)queue);
}

typedef struct tmr_adapter
{
    TimerHandle_t timer;
    ezos_thread_timer_cb func;
    void *func_arg;
    ezos_timer_type_t type;
    ezos_timer_stat_t stat;
} timer_adapter_t;

static void tmr_adapt_cb(TimerHandle_t xTimer)
{
    timer_adapter_t *timer = pvTimerGetTimerID(xTimer);

    timer->func(timer->func_arg);

    if (timer->type == EZOS_TIMER_TYPE_ONCE)
    {
        timer->stat = EZOS_TIMER_ST_INACTIVE;
    }
}

//  ==== timer Management Functions====
/// @brief 创建一个定时器
/// @param cb 定时器回调函数
/// @param arg  定时器回调函数参数
/// @param repeat 周期或单次（1：周期，0：单次）。
/// @return 返回一个定时器，失败返回NULL
ezos_timer_id_t ezos_timer_create(ezos_thread_timer_cb cb, void *arg, int repeat)
{
    ezos_timer_type_t type;
    if (repeat == 0)
    {
        type = EZOS_TIMER_TYPE_ONCE;
    }
    else
    {
        type = EZOS_TIMER_TYPE_PERIODIC;
    }
    timer_adapter_t *tmr_adapter = pvPortMalloc(sizeof(timer_adapter_t));
    if (tmr_adapter == NULL)
    {
        return NULL;
    }

    tmr_adapter->func = cb;
    tmr_adapter->func_arg = arg;
    tmr_adapter->type = type;
    tmr_adapter->stat = EZOS_TIMER_ST_INACTIVE;

    TimerHandle_t handle = xTimerCreate("Timer", 1, type, tmr_adapter, tmr_adapt_cb);
    if (handle != NULL)
    {
        tmr_adapter->timer = handle;
        return tmr_adapter;
    }
    else
    {
        vPortFree(tmr_adapter);
        return NULL;
    }
}

/// @brief 删除定时器
/// @param timer 定时器
ezos_status_t ezos_timer_destroy(ezos_timer_id_t timer)
{
    if (timer == NULL)
    {
        return EZOS_EINVAL;
    }

    timer_adapter_t *tmr_adapter = timer;

    int ret = xTimerDelete(tmr_adapter->timer, DONT_BLOCK);

    if (!ret)
    {
        return EZOS_EPERM;
    }

    vPortFree(tmr_adapter);

    return EZOS_SUCCESS;
}

/// @brief 启动定时器
/// @param timer 定时器
/// @param ms 定时时间 ms不能为0
/// @return  0：成功，非0 失败
ezos_status_t ezos_timer_start(ezos_timer_id_t timer, uint32_t ms)
{
    if (timer == NULL || ms == 0)
    {
        return EZOS_EINVAL;
    }
    timer_adapter_t *tmr_adapter = timer;
    int tmp = xTimerChangePeriod(tmr_adapter->timer, pdMS_TO_TICKS(ms), DONT_BLOCK);
    if (tmp == pdTRUE)
    {
        tmr_adapter->stat = EZOS_TIMER_ST_ACTIVE;
        return EZOS_SUCCESS;
    }
    else
    {
        return EZOS_FAILURE;
    }
}

/// @brief 停止定时器
/// @param timer 定时器
/// @return 0：成功，非0 失败
ezos_status_t ezos_timer_stop(ezos_timer_id_t timer)
{
    if (timer == NULL)
    {
        return EZOS_EINVAL;
    }
    timer_adapter_t *tmr_adapter = timer;
    int tmp;
    tmp = xTimerStop(tmr_adapter->timer, DONT_BLOCK);
    if (tmp == 0)
    {
        return EZOS_ETIMEDOUT;
    }

    if (tmp == pdTRUE)
    {
        tmr_adapter->stat = EZOS_TIMER_ST_INACTIVE;
        return EZOS_SUCCESS;
    }
    else
    {
        return EZOS_FAILURE;
    }
}

/// @brief 停止定时器，并设置定时时间，重新启动
/// @param timer 定时器
/// @param ms新的周期或单次
/// @return 0：成功，非0 失败
ezos_status_t ezos_timer_update(ezos_timer_id_t timer, uint32_t ms)
{
    if (timer == NULL)
    {
        return EZOS_EINVAL;
    }
    timer_adapter_t *tmr_adapter = timer;
    if (tmr_adapter->stat == EZOS_TIMER_ST_ACTIVE)
    {
        return EZOS_EINVAL;
    }
    int tmp = xTimerChangePeriod(tmr_adapter->timer, pdMS_TO_TICKS(ms), DONT_BLOCK);

    if (tmp == pdTRUE)
    {
        tmr_adapter->stat = EZOS_TIMER_ST_ACTIVE;
        return EZOS_SUCCESS;
    }
    else
    {
        return EZOS_FAILURE;
    }
}

/// @brief 检查定时器是否在运行
/// @param timer 定时器
/// @return 0：不在运行，1：正在运行
ezos_timer_stat_t ezos_timer_is_active(ezos_timer_id_t timer)
{
    if (timer == NULL)
    {
        return EZOS_TIMER_ST_INACTIVE;
    }
    timer_adapter_t *tmr_adapter = timer;
    return tmr_adapter->stat;
}

//  ==== system Management Functions====
ezos_status_t ezos_init(void)
{

    return EZOS_SUCCESS;
}

ezos_status_t ezos_start(void)
{
    vTaskStartScheduler();
    return EZOS_SUCCESS;
}

void ezos_delayms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void ezos_delays(uint32_t s)
{
    while (s--)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

const char *ezos_info_get(void)
{
    return EZOS_VERSION_INFO;
}

uint32_t ezos_tick_conut_get(void)
{
    TickType_t ticks;

    if (ezos_irq_context() != 0U)
    {
        ticks = xTaskGetTickCountFromISR();
    }
    else
    {
        ticks = xTaskGetTickCount();
    }

    return ticks;
}

uint32_t ezos_tick_freq_get(void)
{
    return configTICK_RATE_HZ;
}

void __attribute__((weak)) weak_ezos_puts(char *data)
{
    printf(data);
}

void ezos_printf(const char *fmt, ...)
{
    static char _log_buf[DEBUG_PRINTF_MAX_SIZE];
    memset(_log_buf, 0, DEBUG_PRINTF_MAX_SIZE);
    va_list args;
    va_start(args, fmt);
    vsnprintf(_log_buf, DEBUG_PRINTF_MAX_SIZE, fmt, args);
    va_end(args);
    weak_ezos_puts(_log_buf);
}

#endif // OSAL_USE_FREERTOS