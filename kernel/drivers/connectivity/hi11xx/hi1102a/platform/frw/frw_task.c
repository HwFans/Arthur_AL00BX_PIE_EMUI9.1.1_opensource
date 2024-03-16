

/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include "frw_task.h"

#include "platform_spec.h"
#include "frw_main.h"
#include "frw_event_main.h"
#include "hal_ext_if.h"

#if ((_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)))
#include <linux/signal.h>
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_FRW_TASK_C

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
/******************************************************************************
    �¼�����ȫ�ֱ���
*******************************************************************************/
frw_task_stru event_task[WLAN_FRW_MAX_NUM_CORES];

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/

#if (_PRE_FRW_FEATURE_PROCCESS_ENTITY_TYPE == _PRE_FRW_FEATURE_PROCCESS_ENTITY_THREAD)

/*
 * �� �� ��  : frw_set_thread_property
 * ��������  : �����̲߳�������
 * �������  : p: ��ǰ�߳�; policy: ���Ȳ���; param:
 * �������  : ��
 * �� �� ֵ  : ��
 */
OAL_STATIC void frw_set_thread_property(oal_task_stru *p, int policy, struct sched_param *param, long nice)
{
    if (p == NULL) {
        OAL_IO_PRINT("oal_task_stru p is null r failed!%s\n", __FUNCTION__);
        return;
    }

    if (param == NULL) {
        OAL_IO_PRINT("param is null r failed!%s\n", __FUNCTION__);
        return;
    }

    if (oal_sched_setscheduler(p, policy, param)) {
        OAL_IO_PRINT("[Error]set scheduler failed! %d\n", policy);
    }

    if (policy != SCHED_FIFO && policy != SCHED_RR) {
        OAL_IO_PRINT("set thread scheduler nice %ld\n", nice);
        oal_set_user_nice(p, nice);
    }
}

/*
 * �� �� ��  : frw_task_thread
 * ��������  : frw �ں��߳�������
 * �������  : ��id
 * �������  : ��
 * �� �� ֵ  : ��
 */
OAL_STATIC oal_int32 frw_task_thread(oal_void *arg)
{
    oal_uint32 ul_bind_cpu = (oal_uint32)(uintptr_t)arg;
    oal_int32 ret = 0;
#if (_PRE_FRW_FEATURE_PROCCESS_ENTITY_TYPE == _PRE_FRW_FEATURE_PROCCESS_ENTITY_THREAD)
    oal_uint32 ul_empty_count = 0;
    const oal_uint32 ul_count_loop_time = 10000;
#endif

    allow_signal(SIGTERM);

    for (;;) {
#if (_PRE_FRW_FEATURE_PROCCESS_ENTITY_TYPE == _PRE_FRW_FEATURE_PROCCESS_ENTITY_THREAD)
        oal_uint32 ul_event_count;
#endif
        if (oal_kthread_should_stop()) {
            break;
        }

        /* stateΪTASK_INTERRUPTIBLE��condition���������߳�������ֱ�������ѽ���waitqueue */
        /*lint -e730*/
#ifdef _PRE_FRW_EVENT_PROCESS_TRACE_DEBUG
        frw_event_last_pc_trace(__FUNCTION__, __LINE__, ul_bind_cpu);
#endif
        ret = OAL_WAIT_EVENT_INTERRUPTIBLE(event_task[ul_bind_cpu].frw_wq,
                                           OAL_TRUE == frw_task_thread_condition_check((oal_uint)ul_bind_cpu));
        /*lint +e730*/
        if (OAL_UNLIKELY(ret == -ERESTARTSYS)) {
            OAL_IO_PRINT("wifi task %s was interrupted by a signal\n", oal_get_current_task_name());
            break;
        }
#if (_PRE_FRW_FEATURE_PROCCESS_ENTITY_TYPE == _PRE_FRW_FEATURE_PROCCESS_ENTITY_THREAD)
        event_task[ul_bind_cpu].ul_total_loop_cnt++;

        ul_event_count = event_task[ul_bind_cpu].ul_total_event_cnt;
#endif
        frw_event_process_all_event((oal_uint)ul_bind_cpu);
#if (_PRE_FRW_FEATURE_PROCCESS_ENTITY_TYPE == _PRE_FRW_FEATURE_PROCCESS_ENTITY_THREAD)
        if (ul_event_count == event_task[ul_bind_cpu].ul_total_event_cnt) {
            /* ��ת */
            ul_empty_count++;
            if (ul_empty_count == ul_count_loop_time) {
                DECLARE_DFT_TRACE_KEY_INFO("frw_sched_too_much", OAL_DFT_TRACE_EXCEP);
            }
        } else {
            if (ul_empty_count > event_task[ul_bind_cpu].ul_max_empty_count) {
                event_task[ul_bind_cpu].ul_max_empty_count = ul_empty_count;
            }
            ul_empty_count = 0;
        }
#endif
#ifdef _PRE_FRW_EVENT_PROCESS_TRACE_DEBUG
        frw_event_last_pc_trace(__FUNCTION__, __LINE__, ul_bind_cpu);
#endif
    }

    return 0;
}

/*
 * �� �� ��  : frw_task_init
 * ��������  : frw task��ʼ���ӿ�
 */
oal_uint32 frw_task_init(oal_void)
{
    oal_uint ul_core_id;
    oal_task_stru *pst_task;
    struct sched_param param = {0};

    memset_s(event_task, OAL_SIZEOF(event_task), 0, OAL_SIZEOF(event_task));

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        OAL_WAIT_QUEUE_INIT_HEAD(&event_task[ul_core_id].frw_wq);

        pst_task = oal_kthread_create(frw_task_thread, (oal_void *)(uintptr_t) ul_core_id, "hisi_frw/%lu", ul_core_id);
        if (OAL_IS_ERR_OR_NULL(pst_task)) {
            return OAL_FAIL;
        }

        oal_kthread_bind(pst_task, ul_core_id);

        event_task[ul_core_id].pst_event_kthread = pst_task;
        event_task[ul_core_id].uc_task_state = FRW_TASK_STATE_IRQ_UNBIND;

        param.sched_priority = 1;
        frw_set_thread_property(pst_task, SCHED_FIFO, &param, 0);

        oal_wake_up_process(event_task[ul_core_id].pst_event_kthread);
    }

    return OAL_SUCC;
}

/*
 * �� �� ��  : frw_task_exit
 * ��������  : frw task�˳�����
 */
oal_void frw_task_exit(oal_void)
{
    oal_uint32 ul_core_id;

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        if (event_task[ul_core_id].pst_event_kthread) {
            oal_kthread_stop(event_task[ul_core_id].pst_event_kthread);
            event_task[ul_core_id].pst_event_kthread = OAL_PTR_NULL;
        }
    }
}

/*
 * �� �� ��  : frw_task_event_handler_register
 * ��������  : ���ⲿģ��ע��tasklet����������ִ�еĺ����
 */
oal_void frw_task_event_handler_register(oal_void (*p_func)(oal_uint))
{
}

/*
 * �� �� ��  : frw_task_sched
 * ��������  : ����eventʱ�䴦���̣߳���wake_event_interruptible��Ӧ
 */
oal_void frw_task_sched(oal_uint32 ul_core_id)
{
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&event_task[ul_core_id].frw_wq);
}

/*
 * �� �� ��  : frw_task_set_state
 * ��������  : �����ں��̵߳İ�״̬
 */
oal_void frw_task_set_state(oal_uint32 ul_core_id, oal_uint8 uc_task_state)
{
    event_task[ul_core_id].uc_task_state = uc_task_state;
}

/*
 * �� �� ��  : frw_task_get_state
 * ��������  : ��ȡ�ں��̵߳İ�״̬
 */
oal_uint8 frw_task_get_state(oal_uint32 ul_core_id)
{
    return event_task[ul_core_id].uc_task_state;
}

#elif (_PRE_FRW_FEATURE_PROCCESS_ENTITY_TYPE == _PRE_FRW_FEATURE_PROCCESS_ENTITY_TASKLET)

// ʹ��tasklet���к˼�ͨ�ţ�tasklet��ʼ��ʱָ���˼�ͨ�ŷ���
#if WLAN_FRW_MAX_NUM_CORES == 1
#define FRW_DST_CORE(this_core) 0
#elif WLAN_FRW_MAX_NUM_CORES == 2
#define FRW_DST_CORE(this_core) (this_core == 0 ? 1 : 0)
#else
#define FRW_DST_CORE(this_core) 0
#error enhance ipi to support more cores!
#endif

OAL_STATIC oal_void frw_task_ipi_handler(oal_uint ui_arg);

/*
 * �� �� ��  : frw_task_init
 * ��������  : tasklet��ʼ���ӿ�
 */
oal_uint32 frw_task_init(oal_void)
{
    oal_uint32 ul_core_id;

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        oal_task_init(&event_task[ul_core_id].st_event_tasklet,
                      event_task[ul_core_id].p_event_handler_func, 0);
        oal_task_init(&event_task[ul_core_id].st_ipi_tasklet,
                      frw_task_ipi_handler, FRW_DST_CORE(ul_core_id));
    }
    return OAL_SUCC;
}

/*
 * �� �� ��  : frw_task_exit
 * ��������  : task �˳�����
 */
oal_void frw_task_exit(oal_void)
{
    oal_uint32 ul_core_id;

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        oal_task_kill(&event_task[ul_core_id].st_event_tasklet);
        oal_task_kill(&event_task[ul_core_id].st_ipi_tasklet);
    }
}

/*
 * �� �� ��  : frw_task_event_handler_register
 * ��������  : ���ⲿģ��ע��tasklet����������ִ�еĺ���
 */
oal_void frw_task_event_handler_register(oal_void (*p_func)(oal_uint))
{
    oal_uint32 ul_core_id;

    if (OAL_UNLIKELY(OAL_PTR_NULL == p_func)) {
        OAM_ERROR_LOG0(0, OAM_SF_FRW, "{frw_task_event_handler_register:: p_func is null ptr}");
        return;
    }

    for (ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++) {
        event_task[ul_core_id].p_event_handler_func = p_func;
    }
}

/*
 * �� �� ��  : frw_remote_task_receive
 * ��������  : ��tasklet����ִ�У���IPI�жϵ���ִ��
 */
OAL_STATIC oal_void frw_remote_task_receive(void *info)
{
    oal_tasklet_stru *pst_task = (oal_tasklet_stru *)info;
    oal_task_sched(pst_task);
}

/*
 * �� �� ��  : frw_task_ipi_handler
 * ��������  : ʹ��IPI�жϣ�����Ŀ��core�ϵ�taskletִ�д����¼�
 */
OAL_STATIC oal_void frw_task_ipi_handler(oal_uint ui_arg)
{
    oal_uint32 ul_this_id   = OAL_GET_CORE_ID();
    oal_uint32 ul_remote_id = (oal_uint32)ui_arg;

    if (ul_this_id == ul_remote_id) {
        OAL_IO_PRINT("BUG:this core = remote core = %d\n", ul_this_id);
        return;
    }

    oal_smp_call_function_single(ul_remote_id, frw_remote_task_receive,
                                 &event_task[ul_remote_id].st_event_tasklet, 0);
}

/*
 * �� �� ��  : frw_task_sched
 * ��������  : task���Ƚӿ�
 */
oal_void frw_task_sched(oal_uint32 ul_core_id)
{
    oal_uint32 ul_this_cpu = OAL_GET_CORE_ID();

    if (oal_task_is_scheduled(&event_task[ul_core_id].st_event_tasklet)) {
        return;
    }

    if (ul_this_cpu == ul_core_id) {
        oal_task_sched(&event_task[ul_core_id].st_event_tasklet);
    } else {
        if (oal_task_is_scheduled(&event_task[ul_this_cpu].st_ipi_tasklet)) {
            return;
        }
        oal_task_sched(&event_task[ul_this_cpu].st_ipi_tasklet);
    }
}

/*
 * �� �� ��  : frw_task_set_state
 * ��������  : ����tasklet��״̬
 */
oal_void frw_task_set_state(oal_uint32 ul_core_id, oal_uint8 uc_task_state)
{
}

/*
 * �� �� ��  : frw_task_get_state
 * ��������  : ��ȡtasklet��״̬��taskletһֱ��˰�
 */
oal_uint8 frw_task_get_state(oal_uint32 ul_core_id)
{
    return FRW_TASK_STATE_IRQ_BIND;
}

#endif

#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
oal_module_symbol(frw_task_set_state);
oal_module_symbol(frw_task_get_state);
#endif

