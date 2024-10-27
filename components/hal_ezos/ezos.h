#ifndef __EASY_OSAL_H__
#define __EASY_OSAL_H__

#include <stdint.h>
#include <stddef.h>


#define EZOS_VERSION_INFO "EasyOSAL Version 0.0.4(beta)"

#define EZ_MAX_PRIORITY 32
#define EZ_DEFAULT_PRIORITY 16
#define EZ_CONFIGMINIMAL_STACK_SIZE    ((unsigned short) (512))

#define DEBUG_PRINTF_MAX_SIZE 128

typedef void (*ezos_thread_func_cb)(void *arg);
typedef void (*ezos_thread_timer_cb)(void *arg);

typedef void *ezos_thread_id_t;
typedef void *ezos_mutex_id_t;
typedef void *ezos_sem_id_t;
typedef void *ezos_queue_id_t;
typedef void *ezos_timer_id_t;

#define EZOS_DELAY_FOREVER 0xffffffff


// status code
typedef enum
{
    EZOS_FAILURE = -1,
    EZOS_SUCCESS,
    EZOS_EINVAL, // Invalid argument
    EZOS_EPERM,  // operation not permitted
    EZOS_ERRISR,
    EZOS_TIMEOUT,   // timeout
    EZOS_EBUZY,     //
    EZOS_ETIMEDOUT, // connection time out
} ezos_status_t;

/// timer state.
typedef enum
{
    EZOS_TIMER_ST_INACTIVE = 0, /// not running
    EZOS_TIMER_ST_ACTIVE = 1,   /// running
} ezos_timer_stat_t;

/// Timer type.
typedef enum
{
    EZOS_TIMER_TYPE_ONCE = 0,    /// One-shot timer.
    EZOS_TIMER_TYPE_PERIODIC = 1 /// Repeating timer.
} ezos_timer_type_t;

typedef struct
{
    char *thread_name;   // 任务名称
    void *user_arg;      // 传递参数
    uint16_t priority;   // 优先级
    uint32_t stack_size; // 任务栈大小
} ezos_thread_params_t;

//  ==== Thread Functions ====
/// @brief 创建任务,
/// @param func   任务函数
/// @param param   任务处理参数,如果参数为空，使用系统默认值，简化系统函数传入参数
/// @return    返回值，如果为NULL，则失败，否则成功
ezos_thread_id_t ezos_thread_create(ezos_thread_func_cb func, ezos_thread_params_t *param);

/// @brief 删除任务
/// @param id 线程id
void ezos_thread_destroy(ezos_thread_id_t id);

/// @brief 任务挂起
/// @param 线程id
/// @return  0：success
ezos_status_t ezos_thread_suspend(ezos_thread_id_t id);

/// @brief 恢复任务挂起
/// @param 线程id
/// @return  0：success
ezos_status_t ezos_thread_resume(ezos_thread_id_t id);

/// @brief 放弃时间片
/// @return  0：success
ezos_status_t ezos_thread_yield(void);

//  ==== Memory Functions ====
/// @brief 内存分配
/// @param size 分配大小
/// @return 返回分配空间的地址，不为NULL，则成功
void *ezos_malloc(uint32_t size);

/// @brief  为当前内存重新申请空间
/// @param ptr 指针指向一个要重新分配内存的内存块
/// @param size 重新分配大小
/// @return 返回重新分配空间的地址，不为NULL，则成功
void *ezos_realloc(void *ptr, uint32_t size);

/// @brief  分配所需的内存空间，并返回一个指向它的指针，使用calloc分配，内存会默认初始化为0
/// @param nitems 要被分配的元素个数
/// @param size  元素的大小
/// @return 回分配空间的地址，不为NULL，则成功
void *ezos_calloc(uint32_t nitems, size_t size);

/// @brief 释放之前调用 calloc、malloc 或 realloc 所分配的内存空间。
/// @param ptr 指针指向一个要释放内存的内存块
void ezos_free(void *ptr);

//  ==== mutex Functions ====
/// @brief 创建锁
/// @return 返回NULL，则失败
ezos_mutex_id_t ezos_mutex_create(void);

/// @brief 删除锁
/// @param mutex 锁句柄
ezos_status_t ezos_mutex_destroy(ezos_mutex_id_t mutex);

/// @brief 锁住
/// @param mutex 锁句柄
ezos_status_t ezos_mutex_lock(ezos_mutex_id_t mutex);

/// @brief 解锁
/// @param mutex 锁句柄
ezos_status_t ezos_mutex_unlock(ezos_mutex_id_t mutex);

//  ==== Semaphore Management Function ====
/// @brief 创建信号量
/// @param max_count   信号量最大计数器
/// @param initial_count    信号量计数器初始值
/// @return 成功，返回一个信号量，失败返回NULL
ezos_sem_id_t ezos_sem_create(uint32_t max_count, uint32_t initial_count);

/// @brief 删除信号量
/// @param sem 信号量
ezos_status_t ezos_sem_destroy(ezos_sem_id_t sem);

/// @brief 等待获取信号量，获取不到，当前阻塞
/// @param sem  信号量
/// @param timeout 超时时间。传入0表示不超时，立即返回；0xFFFFFFFFFF表示永久等待;其他数值表示超时时间，单位ms。
/// @return    - 0: 成功，-1：失败，其他：返回超时
ezos_status_t ezos_sem_take(ezos_sem_id_t sem, uint32_t timeout);

/// @brief 释放信号量
/// @param sem 信号量
ezos_status_t ezos_sem_give(ezos_sem_id_t sem);

//  ==== Message Queue Management Functions====
/// @brief 创建一个消息队列
/// @param queue_count 队列消息数量
/// @param queue_size   队列中最大消息大小
/// @return 成功返回一个队列，失败返回NULL
ezos_queue_id_t ezos_queue_create(uint32_t msg_count, uint32_t msg_size);

/// @brief 删除并释放队列
/// @param queue 需要释放的队列
void ezos_queue_destroy(ezos_queue_id_t queue);

/// @brief 将消息写入队列中
/// @param queue 队列
/// @param msg 消息
/// @param timeout 队列满超时时间。传入0表示不超时，立即返回；0xFFFFFFFFFF表示永久等待;其他时间表示超时时间
/// @return  0: 成功，-1：失败，其他：返回超时
ezos_status_t ezos_queue_write(ezos_queue_id_t queue, void *msg_ptr,uint32_t msg_size, uint32_t timeout);

/// @brief 从队列中读取消息
/// @param queue 队列
/// @param msg_ptr 消息
/// @param timeout 读队列空超时时间。传入0表示不超时，立即返回；0xFFFFFFFFFF表示永久等待;其他时间表示超时时间
/// @return  0: 成功，-1：失败，其他：返回超时
ezos_status_t ezos_queue_read(ezos_queue_id_t queue, void *msg_ptr,uint32_t msg_size,uint32_t timeout);

/// @brief 获取有效消息个数
/// @param queue 队列
/// @return 返回有效消息个数
uint32_t ezos_queue_count_get(ezos_queue_id_t queue);

/// @brief 重置队列
/// @param queue 队列
void ezos_queue_reset(ezos_queue_id_t queue);

//  ==== timer Management Functions====
/// @brief 创建一个定时器
/// @param cb 定时器回调函数
/// @param arg  定时器回调函数参数
/// @param repeat 周期或单次（1：周期，0：单次）。
/// @return 返回一个定时器，失败返回NULL
ezos_timer_id_t ezos_timer_create(ezos_thread_timer_cb cb, void *arg, int repeat);

/// @brief 删除定时器
/// @param timer 定时器
ezos_status_t ezos_timer_destroy(ezos_timer_id_t timer);

/// @brief 启动定时器
/// @param timer 定时器
/// @param ms 定时器定时时间
/// @return  0：成功，非0 失败
ezos_status_t ezos_timer_start(ezos_timer_id_t timer, uint32_t ms);

/// @brief 停止定时器
/// @param timer 定时器
/// @return 0：成功，非0 失败
ezos_status_t ezos_timer_stop(ezos_timer_id_t timer);

/// @brief 停止定时器，并设置定时时间，重新启动
/// @param timer 定时器
/// @param ms 新的周期或单次
/// @return 0：成功，非0 失败
ezos_status_t ezos_timer_update(ezos_timer_id_t timer, uint32_t ms);

/// @brief 检查定时器是否在运行
/// @param timer 定时器
/// @return 0：不在运行，1：正在运行
ezos_timer_stat_t ezos_timer_is_active(ezos_timer_id_t timer);

//  ==== system Management Functions====
/// @brief os系统初始化
ezos_status_t ezos_init(void);

/// @brief os系统开始调度
ezos_status_t ezos_start(void);

/// @brief os系统延时毫秒
/// @param ms 单位毫秒
void ezos_delayms(uint32_t ms);

/// @brief os系统延时秒
/// @param ms 单位秒
void ezos_delays(uint32_t s);

/// @brief 获取系统版本信息
/// @return 返回系统版本信息
const char *ezos_info_get(void);

/// @brief 内核调度挂起
/// @return 大于等于0: 内核挂起的TICK数。
uint32_t ezos_suspend(void);

/// @brief 恢复内核调度
/// @param sleep_ticks 需要多少TICK数恢复
void ezos_resume(int32_t sleep_ticks);

/// @brief 获取OS运行起的tick数
/// @return 返回OS运行起的tick数
uint32_t ezos_tick_conut_get(void);

/// @brief 获取周期tick数
/// @return 返回周期tick数
uint32_t ezos_tick_freq_get(void);

/// @brief 打印
/// @param fmt 
/// @param  
void ezos_printf(const char *fmt, ...);

#endif /* __EASY_OSAL_H__ */
