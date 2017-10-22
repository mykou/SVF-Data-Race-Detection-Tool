#ifndef HARE_HARE_H
#define HARE_HARE_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

#define OMP_LIKE_VERSION 1
#define TBB_LIKE_VERSION 0

/**
* @ingroup  hare
*
* 表示最大可创建线程数
*/
extern int g_max_worker_num;

/**
* @ingroup  hare
*
*hare锁类型
*/
class Lock;
typedef Lock *Lock_h;

/**
*@ingroup hare
*@brief 初始化hare环境。
*
*@par 描述:
*创建worker_pool, team_list, partitioner,即为它们申请相应的内存空间。\n
*worker_pool为用于存储线程的线程池对象，team_list用于线程分组，partitioner为用于任务划分\n
*本接口在并行区域之前调用，后续会增加线程安全保证
*
*@attention 本接口只可以被调用一次。
*
*@param
*
*@retval
*@par 依赖：
*@li hare.h: 该接口声明所在头文件。
*@see hare_finish
*/
EXTERNC void hare_init();

/**
*@ingroup hare
*@brief 销毁hare环境。
*
*@par 描述:
*销毁线程池中的所有线程并销毁线程池，销毁team_list,partitioner.
*
*@attention 本接口只可以被调用一次，调用后hare环境不可用
*
*@param
*
*@retval
*@par 依赖：
*@li hare.h: 该接口声明所在头文件。
*@see hare_init
*/
EXTERNC void hare_finish();

/**
*@ingroup hare
*@brief parallel_for pattern。
*
*@par 描述:
*用于并行循环区域，包括while/for循环。\n
*调度类型，选项：静态调度和动态调度\n
*1) 静态调度，调度时采用按任务数均匀发放的策略，给每个计算线程平均分配任务；\n
*2)
*动态调度，调度时采用能者多劳的策略，先给每个计算线程发一批任务，当某个计算线程完成任务后，\n
*再给该线程发送下一批任务；
*本操作非线程安全，后续会考虑添加线程安全保证
*
*@attention
*对于循环内部存在break或者其它提前退出循环的语句，无法用hare_parallel_for并行，需调整算法。
*
*@param chunk_size [IN]  任务粒度大小，默认为1。
*@param start [IN] 循环开始下标。
*@param end [IN] 循环结束下标，如果循环结束为<=某个数，那么end需要+1。
*@param incur [IN] 循环增加频率，如i++,频率为1，i+=2为频率为2，默认为1。
*@param fn [IN]
*任务函数，单个线程执行的任务，其中第一个参数为数据参数，后两个分别为循环起始
*\n
*和结束下标，表示分配给线程的任务大小。
*@param data [IN] 数据参数，同参数fn。
*@param thread_num [IN]
*线程数，用户指定并行线程个数，如果用户指定的线程个数超过系统所拥有的逻辑核心数，则线程数为系统\n
*逻辑核心数；如果用户传入参数小于等于0，线程数同样为系统逻辑核心数。
*
*@retval
*@par 依赖：
*@li hare.h: 该接口声明所在头文件。
*@see
*/
EXTERNC void hare_parallel_for(long chunk_size, long start, long end, long incr,
                               void (*fn)(void *, long, long), void *data,
                               int thread_num, int schedule);
/**
*@ingroup hare
*@brief 获取线程号
*
*@par 描述:
*获取当前运行线程的线程id,从0开始
*
*@attention 本接口只可在hare_parallel_for并行的区域内调用
*
*@param
*
*@retval 该线程id号
*@par 依赖：
*@li hare.h: 该接口声明所在头文件。
*@see
*/
EXTERNC int hare_get_thread_id();

/**
*@ingroup hare
*@brief 初始化锁
*
*@par 描述:
*初始化锁
*
*@attention
*
*@param
*
*@retval 创建的锁
*@par 依赖：
*@li hare.h: 该接口声明所在头文件。
*@see
*/
EXTERNC Lock_h hare_lock_init();

/**
*@ingroup hare
*@brief 销毁锁
*
*@par 描述:
*销毁锁
*
*@attention
*
*@param [IN] lock 创建的锁
*
*@retval 创建的锁
*@par 依赖：
*@li hare.h: 该接口声明所在头文件。
*@see
*/
EXTERNC void hare_lock_destroy(Lock_h lock);

/**
*@ingroup hare
*@brief 加锁
*
*@par 描述:
*加锁
*
*@attention
*
*@param [IN] lock 创建的锁
*
*@retval
*@par 依赖：
*@li hare.h: 该接口声明所在头文件。
*@see hare_unlock
*/
EXTERNC void hare_lock(Lock_h lock);

/**
*@ingroup hare
*@brief 解锁
*
*@par 描述:
*解锁
*
*@attention
*
*@param [IN] lock 创建的锁
*
*@retval
*@par 依赖：
*@li hare.h: 该接口声明所在头文件。
*@see hare_lock
*/
EXTERNC void hare_unlock(Lock_h lock);
/**
*@ingroup hare
*@brief 获取系统时间
*
*@par 描述:
*获取系统时间
*
*@attention
*
*@param [IN] type 时间类型us，ms，s
*
*@retval 具体时间
*@par 依赖：
*@li hare.h: 该接口声明所在头文件。
*@see
*/
EXTERNC long hare_get_time(int type);
/**
*@ingroup hare
*@brief 获取线程具体执行硬件core id.
*
*@par 描述:
*获取线程具体执行硬件core id。
*
*@attention
*
*@param
*
*@retval 具体core id
*@par 依赖：
*@li hare.h: 该接口声明所在头文件。
*@see
*/
EXTERNC int hare_get_cpu_id();
#undef EXTERNC
#endif
