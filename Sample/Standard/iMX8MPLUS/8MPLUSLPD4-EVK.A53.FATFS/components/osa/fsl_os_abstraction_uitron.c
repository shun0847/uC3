/*! *********************************************************************************
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2019, 2025 NXP
 *
 *
 * This is the source file for the OS Abstraction layer for uitron.
 * SPDX-License-Identifier: BSD-3-Clause
 ********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "fsl_common.h"
#include "fsl_os_abstraction.h"
#include "fsl_os_abstraction_uitron.h"
#include <string.h>

/*! *********************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
********************************************************************************** */

/* Weak function. */
#if defined(__GNUC__)
#define __WEAK_FUNC __attribute__((weak))
#elif defined(__ICCARM__)
#define __WEAK_FUNC __weak
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define __WEAK_FUNC __attribute__((weak))
#endif

#ifdef DEBUG_ASSERT
#define OS_ASSERT(condition) \
    if (!(condition))        \
        while (1)            \
            ;
#else
#define OS_ASSERT(condition) (void)(condition);
#endif

/*! @brief Converts milliseconds to ticks*/
#define MSEC_TO_TICKS(msec) (((msec) * (uint32_t)(configTICK_RATE_HZ) + 999U) / 1000U)
#define TICKS_TO_MSEC(tick) ((uint32_t)((uint64_t)(tick)*1000uL / (uint64_t)configTICK_RATE_HZ))

#define OSA_MEM_MAGIC_NUMBER (12345U)
#define OSA_MEM_SIZE_ALIGN(var, alignbytes) \
    ((unsigned int)((var) + ((alignbytes)-1U)) & (unsigned int)(~(unsigned int)((alignbytes)-1U)))

/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/
typedef struct _osa_id_handle
{
    ID id;
} osa_id_handle_t;

typedef struct _osa_event_handle
{
    ID id;
    uint8_t autoClear;
    uint8_t reserved[3];
} osa_event_handle_struct_t;

/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
********************************************************************************** */
static inline osa_id_handle_t *osa_id_handle(osa_semaphore_handle_t handle)
{
    return (osa_id_handle_t *)handle;
}

static inline osa_event_handle_struct_t *osa_event_handle(osa_event_handle_t handle)
{
    return (osa_event_handle_struct_t *)handle;
}

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */
/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_MemoryAllocate
 * Description   : Reserves the requested amount of memory in bytes.
 *
 *END**************************************************************************/
void *OSA_MemoryAllocate(uint32_t memLength)
{
    void *p = malloc(memLength);
    if (p != NULL)
    {
        (void)memset(p, 0, memLength);
    }
    return p;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_MemoryFree
 * Description   : Frees the memory previously reserved.
 *
 *END**************************************************************************/
void OSA_MemoryFree(void *p)
{
    free(p);
}

void *OSA_MemoryAllocateAlign(uint32_t memLength, uint32_t alignbytes)
{
    (void)alignbytes;
    return OSA_MemoryAllocate(memLength);
}

void OSA_MemoryFreeAlign(void *p)
{
    OSA_MemoryFree(p);
}

void OSA_EnterCritical(uint32_t *sr)
{
    (void)sr;
    (void)loc_cpu();
}

void OSA_ExitCritical(uint32_t sr)
{
    (void)sr;
    (void)unl_cpu();
}

void OSA_TimeDelay(uint32_t millisec)
{
    if (millisec == 0U)
    {
        return;
    }
    (void)dly_tsk((RELTIM)millisec);
}

uint32_t OSA_TimeGetMsec(void)
{
    SYSTIM st;
    if (get_tim(&st) != E_OK)
    {
        return 0U;
    }
    return (uint32_t)st.ltime;
}

osa_status_t OSA_SemaphorePrecreate(osa_semaphore_handle_t semaphoreHandle, osa_task_ptr_t taskHandler)
{
    (void)semaphoreHandle;
    (void)taskHandler;
    return KOSA_StatusSuccess;
}

osa_status_t OSA_SemaphoreCreate(osa_semaphore_handle_t semaphoreHandle, uint32_t initValue)
{
    osa_id_handle_t *h = osa_id_handle(semaphoreHandle);
    T_CSEM csem;
    csem.sematr = TA_TFIFO;
    csem.isemcnt = initValue;
    csem.maxsem = 0xFFU;
    csem.name = NULL;
    h->id = acre_sem(&csem);
    return (h->id > 0) ? KOSA_StatusSuccess : KOSA_StatusError;
}

osa_status_t OSA_SemaphoreCreateBinary(osa_semaphore_handle_t semaphoreHandle)
{
    return OSA_SemaphoreCreate(semaphoreHandle, 0U);
}

osa_status_t OSA_SemaphoreDestroy(osa_semaphore_handle_t semaphoreHandle)
{
    osa_id_handle_t *h = osa_id_handle(semaphoreHandle);
    return (del_sem(h->id) == E_OK) ? KOSA_StatusSuccess : KOSA_StatusError;
}

osa_status_t OSA_SemaphoreWait(osa_semaphore_handle_t semaphoreHandle, uint32_t millisec)
{
    osa_id_handle_t *h = osa_id_handle(semaphoreHandle);
    TMO tmo = (millisec == osaWaitForever_c) ? TMO_FEVR : (TMO)millisec;
    ER ercd = twai_sem(h->id, tmo);
    if (ercd == E_OK)
    {
        return KOSA_StatusSuccess;
    }
    if (ercd == E_TMOUT)
    {
        return KOSA_StatusTimeout;
    }
    return KOSA_StatusError;
}

osa_status_t OSA_SemaphorePost(osa_semaphore_handle_t semaphoreHandle)
{
    osa_id_handle_t *h = osa_id_handle(semaphoreHandle);
    return (sig_sem(h->id) == E_OK) ? KOSA_StatusSuccess : KOSA_StatusError;
}

osa_semaphore_count_t OSA_SemaphoreGetCount(osa_semaphore_handle_t semaphoreHandle)
{
    osa_id_handle_t *h = osa_id_handle(semaphoreHandle);
    T_RSEM rsem;
    if (ref_sem(h->id, &rsem) != E_OK)
    {
        return 0U;
    }
    return (osa_semaphore_count_t)rsem.semcnt;
}

osa_status_t OSA_MutexCreate(osa_mutex_handle_t mutexHandle)
{
    osa_id_handle_t *h = (osa_id_handle_t *)mutexHandle;
    T_CMTX cmtx;
    cmtx.mtxatr = TA_TFIFO;
    cmtx.ceilpri = 0U;
    cmtx.name = NULL;
    h->id = acre_mtx(&cmtx);

    return (h->id > 0) ? KOSA_StatusSuccess : KOSA_StatusError;
}

osa_status_t OSA_MutexLock(osa_mutex_handle_t mutexHandle, uint32_t millisec)
{
    osa_id_handle_t *h = (osa_id_handle_t *)mutexHandle;
    TMO tmo = (millisec == osaWaitForever_c) ? TMO_FEVR : (TMO)millisec;
    ER ercd = tloc_mtx(h->id, tmo);
    if (ercd == E_OK)
    {
        return KOSA_StatusSuccess;
    }
    if (ercd == E_TMOUT)
    {
        return KOSA_StatusTimeout;
    }
    return KOSA_StatusError;
}

osa_status_t OSA_MutexUnlock(osa_mutex_handle_t mutexHandle)
{
    osa_id_handle_t *h = (osa_id_handle_t *)mutexHandle;
    return (unl_mtx(h->id) == E_OK) ? KOSA_StatusSuccess : KOSA_StatusError;
}

osa_status_t OSA_MutexDestroy(osa_mutex_handle_t mutexHandle)
{
    osa_id_handle_t *h = (osa_id_handle_t *)mutexHandle;
    return (del_mtx(h->id) == E_OK) ? KOSA_StatusSuccess : KOSA_StatusError;
}

osa_status_t OSA_EventPrecreate(osa_event_handle_t eventHandle, osa_task_ptr_t taskHandler)
{
    (void)eventHandle;
    (void)taskHandler;
    return KOSA_StatusSuccess;
}

osa_status_t OSA_EventCreate(osa_event_handle_t eventHandle, uint8_t autoClear)
{
    osa_event_handle_struct_t *h = osa_event_handle(eventHandle);
    T_CFLG cflg;
    cflg.flgatr = TA_TFIFO | TA_WMUL | ((autoClear != 0U) ? TA_CLR : 0U);
    cflg.iflgptn = 0U;
    cflg.name = NULL;
    h->id = acre_flg(&cflg);
    h->autoClear = autoClear;
    return (h->id > 0) ? KOSA_StatusSuccess : KOSA_StatusError;
}

osa_status_t OSA_EventSet(osa_event_handle_t eventHandle, osa_event_flags_t flagsToSet)
{
    osa_event_handle_struct_t *h = osa_event_handle(eventHandle);
    return (set_flg(h->id, (FLGPTN)flagsToSet) == E_OK) ? KOSA_StatusSuccess : KOSA_StatusError;
}

osa_status_t OSA_EventClear(osa_event_handle_t eventHandle, osa_event_flags_t flagsToClear)
{
    osa_event_handle_struct_t *h = osa_event_handle(eventHandle);
    return (clr_flg(h->id, (FLGPTN)flagsToClear) == E_OK) ? KOSA_StatusSuccess : KOSA_StatusError;
}

osa_status_t OSA_EventGet(osa_event_handle_t eventHandle,
                          osa_event_flags_t flagsToGet,
                          osa_event_flags_t *pSetFlags)
{
    osa_event_handle_struct_t *h = osa_event_handle(eventHandle);
    T_RFLG rflg;
    if (pSetFlags == NULL)
    {
        return KOSA_StatusError;
    }
    if (ref_flg(h->id, &rflg) != E_OK)
    {
        return KOSA_StatusError;
    }
    *pSetFlags = (osa_event_flags_t)(rflg.flgptn & flagsToGet);
    return KOSA_StatusSuccess;
}

osa_status_t OSA_EventWait(osa_event_handle_t eventHandle,
                           osa_event_flags_t flagsToWait,
                           uint8_t waitAll,
                           uint32_t millisec,
                           osa_event_flags_t *pSetFlags)
{
    osa_event_handle_struct_t *h = osa_event_handle(eventHandle);
    MODE mode = (waitAll != 0U) ? TWF_ANDW : TWF_ORW;
    TMO tmo = (millisec == osaWaitForever_c) ? TMO_FEVR : (TMO)millisec;
    FLGPTN outptn = 0U;
    if (pSetFlags == NULL)
    {
        return KOSA_StatusError;
    }
    ER ercd = twai_flg(h->id, (FLGPTN)flagsToWait, mode, &outptn, tmo);
    if (ercd == E_OK)
    {
        *pSetFlags = (osa_event_flags_t)outptn;
        return KOSA_StatusSuccess;
    }
    if (ercd == E_TMOUT)
    {
        return KOSA_StatusTimeout;
    }
    return KOSA_StatusError;
}

osa_status_t OSA_EventDestroy(osa_event_handle_t eventHandle)
{
    osa_event_handle_struct_t *h = osa_event_handle(eventHandle);
    return (del_flg(h->id) == E_OK) ? KOSA_StatusSuccess : KOSA_StatusError;
}

osa_status_t OSA_TaskCreate(osa_task_handle_t taskHandle,
                            const osa_task_def_t *thread_def,
                            osa_task_param_t task_param)
{
    (void)taskHandle;
    (void)thread_def;
    (void)task_param;
    return KOSA_StatusError;
}

osa_task_handle_t OSA_TaskGetCurrentHandle(void)
{
    return NULL;
}

void OSA_TaskYield(void)
{
}

osa_task_priority_t OSA_TaskGetPriority(osa_task_handle_t taskHandle)
{
    (void)taskHandle;
    return 0U;
}

osa_status_t OSA_TaskSetPriority(osa_task_handle_t taskHandle, osa_task_priority_t taskPriority)
{
    (void)taskHandle;
    (void)taskPriority;
    return KOSA_StatusError;
}

osa_status_t OSA_TaskDestroy(osa_task_handle_t taskHandle)
{
    (void)taskHandle;
    return KOSA_StatusError;
}

osa_status_t OSA_TaskNotifyGet(osa_notify_time_ms_t waitTime_ms)
{
    (void)waitTime_ms;
    return KOSA_StatusError;
}

osa_status_t OSA_TaskNotifyPost(osa_task_handle_t taskHandle)
{
    (void)taskHandle;
    return KOSA_StatusError;
}

void OSA_Start(void)
{
}

void OSA_Init(void)
{
}
