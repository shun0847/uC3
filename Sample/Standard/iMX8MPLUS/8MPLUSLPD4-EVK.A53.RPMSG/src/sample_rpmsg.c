/**
 * @file    sample_rpmsg.c
 * @brief   rpmsg sample program for uC3
 * @date    2023.01.18
 * @author  Copyright (c) 2023, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2023.01.18) Ogino
 *            Initial version.
 ****************************************************************************
 */
#include "kernel.h"
#include "DDR_COM.h"
#include "openamp/open_amp.h"
#include "platform_info.h"
#include "rsc_table.h"
#include "ampext_shmem.h"
#include "OpenAMP_RPMsg_cfg.h"

extern int init_system(void);
extern void cleanup_system(void);

#define SHUTDOWN_MSG    (0xEF56A55A)

/* Local variables */

/*-----------------------------------------------------------------------------*
 *  RPMSG callbacks setup by remoteproc_resource_init()
 *-----------------------------------------------------------------------------*/
/* Local variables */

static struct rpmsg_endpoint rp_ept[CFG_RPMSG_SVCNO] = { 0 };

volatile static int evt_svc_unbind[CFG_RPMSG_SVCNO] = { 0 };


/**
 *  Callback Function: rpmsg_endpoint_cb
 *
 *  @param[in] rp_svc
 *  @param[in] data
 *  @param[in] len
 *  @param[in] priv
 *  @param[in] src
 */
static int rpmsg_endpoint_cb0(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
    /* service 0 */
    (void)priv;
    (void)src;

    /* On reception of a shutdown we signal the application to terminate */
    if ((*(unsigned int *)data) == SHUTDOWN_MSG) {
        rpmsg_destroy_ept(ept);
        evt_svc_unbind[0] = 1;
        return RPMSG_SUCCESS;
    }

    /* Send data back to master */
    if (rpmsg_send(ept, data, (int)len) < 0) {
        LPERROR("rpmsg_send failed\n");
        return -1;
    }
    return RPMSG_SUCCESS;
}

static int rpmsg_endpoint_cb1(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
    /* service 1 */
    (void)priv;
    (void)src;

    /* On reception of a shutdown we signal the application to terminate */
    if ((*(unsigned int *)data) == SHUTDOWN_MSG) {
        rpmsg_destroy_ept(ept);
        evt_svc_unbind[1] = 1;
        return RPMSG_SUCCESS;
    }

    /* Send data back to master */
    if (rpmsg_send(ept, data, (int)len) < 0) {
        LPERROR("rpmsg_send failed\n");
        return -1;
    }
    return RPMSG_SUCCESS;
}

/**
 *  Callback Function: rpmsg_service_unbind
 *
 *  @param[in] ept
 */
static void rpmsg_service_unbind0(struct rpmsg_endpoint *ept)
{
    (void)ept;
    /* service 0 */
    rpmsg_destroy_ept(&rp_ept[0]);
    memset(&rp_ept[0], 0x0, sizeof(struct rpmsg_endpoint));
    evt_svc_unbind[0] = 1;
    return ;
}

static void rpmsg_service_unbind1(struct rpmsg_endpoint *ept)
{
    (void)ept;
    /* service 1 */  
    rpmsg_destroy_ept(&rp_ept[1]);
    memset(&rp_ept[1], 0x0, sizeof(struct rpmsg_endpoint));
    evt_svc_unbind[1] = 1;
    return ;
}

static void copy_string(volatile uint32_t *base, uint32_t ch)
{

    base[0] = 'H';
    base[1] = 'e';
    base[2] = 'l';
    base[3] = 'l';
    base[4] = 'o';
    base[5] = ch;
    base[6] = 'S';
    base[7] = 'M';
    base[8] = 'E';
    base[9] = 'M';
    base[10] = ' ';
    base[11] = 'u';
    base[12] = 'C';
    base[13] = '3';
    base[14] = '\0';

    return ;
}

#define RTOS_OWNER_VAL 2U
#define NUM_OF_BLKS 2
void SheardMemoryTask(VP_INT exinf)
{
    unsigned long svcno = (unsigned long)exinf;  
    shmem_init_t initparam = { 0 };
    shmem_handler_t handler;
    SHMEM_ER err;
    uint32_t bid, bid2;
    volatile uint32_t *addr, *addr2;

    /* Clear Shmem control  area */
    if (svcno == 0UL) {
        memset((void*)(CFG_SHMEM_CTL_BASE0), 0, CFG_SHMEM_CTL_SIZE0);
        strcpy((char*)initparam.ctlname, CFG_SHMEM_CTL_NAME0);
        strcpy((char*)initparam.blkname, CFG_SHMEM_BLK_NAME0);
        strcpy((char*)initparam.busname, DEV_BUS_NAME);
    } else {
        memset((void*)(CFG_SHMEM_CTL_BASE1), 0, CFG_SHMEM_CTL_SIZE1);
        strcpy((char*)initparam.ctlname, CFG_SHMEM_CTL_NAME1);
        strcpy((char*)initparam.blkname, CFG_SHMEM_BLK_NAME1);
        strcpy((char*)initparam.busname, DEV_BUS_NAME);
    }
    initparam.owner_value = RTOS_OWNER_VAL;
    initparam.flags = SHMEM_INIT_ALL;
    err = ampext_shmem_init(&handler, &initparam);
    if (err) {
        LPRINTF("Error found in initialization of the shared memory API.\n");
        return ;
    }

    err = ampext_shmem_add(handler, (uint8_t*)"test1", 0x1000U);
    if (err) {
        LPRINTF("Error found in adding the shared memory region %s.\n", "test1");
        goto fin0;
    }
    err = ampext_shmem_add(handler, (uint8_t*)"test2", 0x1000U);
    if (err) {
        LPRINTF("Error found in adding the shared memory region %s.\n", "test2");
        goto fin1;
    }

    err = ampext_shmem_get(handler, (uint8_t*)"test1", &bid, (void**)&addr);
    if (err) {
        LPRINTF("Error found in getting the shared memory region %s.\n", "test1");
        goto fin2;
    }
    err = ampext_shmem_get(handler, (uint8_t*)"test2", &bid2, (void**)&addr2);
    if (err) {
        LPRINTF("Error found in getting the shared memory region %s.\n", "test2");
        goto fin3;
    }

    *addr = 1U;

    while(1) {
        err = ampext_shmem_lock(handler, bid);
        if (err) {
            LPRINTF("Shared memory API locking error found.\n");
            break ;
        }
        copy_string(addr, ' ');
        err = ampext_shmem_unlock(handler, bid);
        if (err) {
            LPRINTF("Shared memory API unlocking error found.\n");
            break ;
        }

        err = ampext_shmem_lock(handler, bid2);
        if (err) {
            LPRINTF("Shared memory API locking error found.\n");
            break ;
        }
        copy_string(addr2, '2');
        err = ampext_shmem_unlock(handler, bid2);
        if (err) {
            LPRINTF("Shared memory API unlocking error found.\n");
            break ;
        }
        dly_tsk(0U);
    }

    (void)ampext_shmem_ret(handler, bid2);
fin3:
    (void)ampext_shmem_ret(handler, bid);
fin2:
    (void)ampext_shmem_delete(handler, (uint8_t*)"test2");
fin1:
    (void)ampext_shmem_delete(handler, (uint8_t*)"test1");
fin0:
    (void)ampext_shmem_deinit(handler);
    return ;
}

/*-----------------------------------------------------------------------------*
 *  Application
 *-----------------------------------------------------------------------------*/
int app(struct rpmsg_device *rdev, void *platform, unsigned long svcno)
{
    int ret;

    (void)platform;

    if (svcno == 0UL) {
        ret = rpmsg_create_ept(&rp_ept[0], rdev, CFG_RPMSG_SVC_NAME0,
                       APP_EPT_ADDR, RPMSG_ADDR_ANY,
                       rpmsg_endpoint_cb0,
                       rpmsg_service_unbind0);
        if (ret) {
            LPERROR("Failed to create endpoint.\n");
            return -1;
        }
    } else {
        ret = rpmsg_create_ept(&rp_ept[1], rdev, CFG_RPMSG_SVC_NAME1,
                       APP_EPT_ADDR, RPMSG_ADDR_ANY,
                       rpmsg_endpoint_cb1,
                       rpmsg_service_unbind1);
        if (ret) {
            //LPERROR("Failed to create endpoint.\n");
            return -1;
        }
    }

    LPRINTF("Waiting for events...\n");
    while(1) {
        dly_tsk(0);
        /* we got a shutdown request, exit */
        if (evt_svc_unbind[svcno]) {
            break;
        }
    }

    return 0;
}

/*******************************
   @    Main Task
 *******************************/

void RPMsgTask(VP_INT exinf)
{
    unsigned long proc_id = (unsigned long)exinf;
    unsigned long rsc_id = (unsigned long)exinf;
    struct rpmsg_device *rpdev;
    void *platform;
    int ret;

   ret = platform_init(proc_id, rsc_id, &platform);
    if (ret) {
        LPERROR("Failed to create remoteproc device.\n");
        goto err1;
    } else {
        /* RTOS is Master, but this setting must remote in this release. */
        rpdev = platform_create_rpmsg_vdev(platform,
                    0U,
                    VIRTIO_DEV_SLAVE,
                    NULL,
                    NULL);

        if (!rpdev) {
            LPERROR("Fail, platform_create_rpmsg_vdev.\n");
            metal_log(METAL_LOG_INFO, "Fail, platform_create_rpmsg_vdev.");
            goto err2 ;
        }

        /* Kick the application */
        (void)app(rpdev, platform, proc_id);

        LPRINTF("De-initializating remoteproc\n");
        platform_release_rpmsg_vdev(platform, rpdev);
    }
err2:
    platform_cleanup(platform);
err1:
    return ;
}

T_CTSK const ctsk_shmem0 = {(TA_HLNG|TA_ACT|TA_FPU), (VP_INT)0U, (FP)SheardMemoryTask, 3U, 0x1000U, 0, "SheardMemoryTask"};
T_CTSK const ctsk_shmem1 = {(TA_HLNG|TA_ACT|TA_FPU), (VP_INT)1U, (FP)SheardMemoryTask, 3U, 0x1000U, 0, "SheardMemoryTask"};
T_CTSK const ctsk_main0 = {(TA_HLNG|TA_ACT|TA_FPU), (VP_INT)0U, (FP)RPMsgTask, 3U, 0x1000U, 0, "RPMsgTask"};
T_CTSK const ctsk_main1 = {(TA_HLNG|TA_ACT|TA_FPU), (VP_INT)1U, (FP)RPMsgTask, 3U, 0x1000U, 0, "RPMsgTask"};

void sample_rpmsg(void)
{
    int ret;


    ret = init_system();
    if (ret) {
        return ;
    }

    (void)acre_tsk((T_CTSK*)&ctsk_shmem0);
    (void)acre_tsk((T_CTSK*)&ctsk_shmem1);
    (void)acre_tsk((T_CTSK*)&ctsk_main0);
    (void)acre_tsk((T_CTSK*)&ctsk_main1);

    //cleanup_system();

    return ;
}
