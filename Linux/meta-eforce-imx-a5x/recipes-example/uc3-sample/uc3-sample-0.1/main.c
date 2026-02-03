/*
 * @file    main.c
 * @brief   Main function and RPMSG example application.
 * @date    2021.04.21
 * @author  Copyright (c) 2021, eForce Co., Ltd. All rights reserved.
 * @license SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "metal/alloc.h"
#include "openamp/open_amp.h"
#include "ampext_shmem.h"
#include "ampext_shmem_cfg.h"
#include "platform_info.h"
#include "OpenAMP_RPMsg_cfg.h"

#define SHUTDOWN_MSG    (0xEF56A55A)

/* Payload definition */
struct _payload {
    unsigned long num;
    unsigned long size;
    unsigned char data[];
};

/* Payload information */
struct payload_info {
    int min;
    int max;
    int num;
};

/* Internal functions */
static void rpmsg_name_service_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest);
static void rpmsg_service_unbind(struct rpmsg_endpoint *ept);
static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv);
static int payload_init(struct rpmsg_device *rdev, struct payload_info *pi);

/* Globals */
static struct rpmsg_endpoint volatile *rp_ept = NULL;
static struct _payload *i_payload;
static int rnum = 0;
static int err_cnt = 0;
static int testno = 0;
static char *svc_name = NULL;

/* External functions */
extern void init_system();
extern void cleanup_system();

#define SHMEM_TEST (1)
#define LINUX_OWNER_VAL (1U)
int sharedMemTest(int proc_id)
{
    shmem_handler_t handler;
    SHMEM_ER err = 0;
    uint32_t bid, bid2;
    volatile uint32_t *addr;
    volatile uint32_t *addr2;
    shmem_init_t initparam;
    LPRINTF(" Shared Memory API Test Ch=%d\n", proc_id);

    if (proc_id == 0) {
        strcpy((char*)initparam.ctlname, CFG_SHMEM_CTL_NAME0);
        strcpy((char*)initparam.blkname, CFG_SHMEM_BLK_NAME0);
        strcpy((char*)initparam.busname, DEV_BUS_NAME);
    } else if (proc_id == 1) {
        strcpy((char*)initparam.ctlname, CFG_SHMEM_CTL_NAME1);
        strcpy((char*)initparam.blkname, CFG_SHMEM_BLK_NAME1);
        strcpy((char*)initparam.busname, DEV_BUS_NAME);
    } else {
        strcpy((char*)initparam.ctlname, CFG_SHMEM_CTL_NAME2);
        strcpy((char*)initparam.blkname, CFG_SHMEM_BLK_NAME2);
        strcpy((char*)initparam.busname, DEV_BUS_NAME);
    }

    initparam.owner_value = LINUX_OWNER_VAL;
    initparam.flags = SHMEM_INIT_NONE;
    err = ampext_shmem_init(&handler, &initparam);
    if (err) {
        LPERROR("Error found in initialization of the shared memory API.\n");
        return -1;
    }

    err = ampext_shmem_get(handler, (uint8_t*)"test1", &bid, (void**)&addr);
    if (err != SHMEM_USED) { /* We assume test1 should be initialized and used by uC3 in advance */
        LPERROR("Error found in getting the shared memory region %s.\n", "test1");
        goto fin0;
    }
    err = ampext_shmem_get(handler, (uint8_t*)"test2", &bid2, (void**)&addr2);
    if (err != SHMEM_USED) { /* We assume test2 should be initialized and used by uC3 in advance */
        LPERROR("Error found in getting the shared memory region %s.\n", "test2");
        goto fin1;
    }

    /* main loop */
    while(1) {
        err = ampext_shmem_lock(handler, bid);
        if (err) {
            LPERROR("Shared memory API locking error found.\n");
            break ;
        }
        LPRINTF("addr:%ls\n", addr);
        LPRINTF("err:%d\n", err);
        addr[0] = 'S';
        addr[1] = 'M';
        addr[2] = 'E';
        addr[3] = 'M';
        addr[4] = ' ';
        addr[5] = 'H';
        addr[6] = 'e';
        addr[7] = 'l';
        addr[8] = 'l';
        addr[9] = 'o';
        addr[10] = ' ';
        addr[11] = 'L';
        addr[12] = 'i';
        addr[13] = 'n';
        addr[14] = 'u';
        addr[15] = 'x';
        addr[16] = '\0';
        LPRINTF("addr:%ls\n", addr);
        err = ampext_shmem_unlock(handler, bid);
        if (err) {
            LPERROR("Shared memory API unlocking error found.\n");
            break ;
        }
        LPRINTF("err:%d\n\n", err);

        sleep(1);

        err = ampext_shmem_lock(handler, bid2);
        if (err) {
            LPERROR("Shared memory API locking error found.\n");
            break ;
        }
        LPRINTF("addr2:%ls\n", addr2);
        LPRINTF("err:%d\n", err);
        addr2[0] = 'S';
        addr2[1] = 'M';
        addr2[2] = 'E';
        addr2[3] = 'M';
        addr2[4] = '2';
        addr2[5] = 'H';
        addr2[6] = 'e';
        addr2[7] = 'l';
        addr2[8] = 'l';
        addr2[9] = 'o';
        addr2[10] = ' ';
        addr2[11] = 'L';
        addr2[12] = 'i';
        addr2[13] = 'n';
        addr2[14] = 'u';
        addr2[15] = 'x';
        addr2[16] = '\0';
        LPRINTF("addr2:%ls\n", addr2);
        err = ampext_shmem_unlock(handler, bid2);
        if (err) {
            LPERROR("Shared memory API unlocking error found.\n");
            break ;
        }
        LPRINTF("err:%d\n\n", err);

        sleep(1);
    }

    /* Never reach if something wrong does not happen */
    err = ampext_shmem_ret(handler, bid2);
    if (err) {
        LPERROR("Shared memory API returning error found.\n");
    }
fin1:
    err = ampext_shmem_ret(handler, bid);
    if (err) {
        LPERROR("Shared memory API returning error found.\n");
    }
fin0:
    err = ampext_shmem_deinit(handler);
    if (err) {
        LPERROR("Shared memory API deinitialization error found.\n");
    }

    return -1; /* This function will never return with 0 */
}

/* Application entry point */
int app (struct rpmsg_device *rdev, void *priv, unsigned long svcno)
{
    int ret = 0;
    int shutdown_msg = SHUTDOWN_MSG;
    int i;
    int size;
    int expect_rnum = 0;
    struct payload_info pi = { 0 };

    LPRINTF(" 1 - Send data to remote core, retrieve the echo");
    LPRINTF(" and validate its integrity ..\n");

    /* Initialization of the payload and its related information */
    if ((ret = payload_init(rdev, &pi))) {
        return ret;
    }

    LPRINTF("Remote proc init.\n");

    /* Create RPMsg endpoint */
    if (svcno == 0) {
        svc_name = (char *)CFG_RPMSG_SVC_NAME0;
    } else if (svcno == 1) {
        svc_name = (char *)CFG_RPMSG_SVC_NAME1;
    } else {
        svc_name = (char *)CFG_RPMSG_SVC_NAME2;
    }

    LPRINTF("Remote proc resource initialized.\n");
    while (!rp_ept  || !is_rpmsg_ept_ready(rp_ept))
        platform_poll(priv);

    LPRINTF("RPMSG endpoint is binded with remote.\n");

    for (i = 0, size = pi.min; i < (int)pi.num; i++, size++) {
        i_payload->num = i;
        i_payload->size = size;
     
        /* Mark the data buffer. */
        memset(&(i_payload->data[0]), 0xA5, size);
     
        LPRINTF("sending payload number %lu of size %lu\n",
             i_payload->num, (2 * sizeof(unsigned long)) + size);
     
        ret = rpmsg_send(rp_ept, i_payload,
             (2 * sizeof(unsigned long)) + size);
     
        if (ret < 0) {
            LPRINTF("Error sending data...%d\n", ret);
        break;
        }
        LPRINTF("echo test: sent : %lu\n", (2 * sizeof(unsigned long)) + size);
     
        expect_rnum++;
        do {
            platform_poll(priv);
        } while ((rnum < expect_rnum) && !err_cnt && rp_ept);
    }

    LPRINTF("************************************\n");
    LPRINTF(" Test Results: Error count = %d \n", err_cnt);
    LPRINTF("************************************\n");
    /* Send shutdown message to remote */
    rpmsg_send(rp_ept, &shutdown_msg, sizeof(unsigned int));
    while (rp_ept) {
        platform_poll(priv);
        LPRINTF("************************************\n");
    }
    LPRINTF("Quitting application .. Echo test end\n");

    metal_free_memory(i_payload);
    return 0;
}

static void rpmsg_name_service_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
    LPRINTF("new endpoint notification is received.\n");
    if (strcmp(name, svc_name)) {
        LPERROR("Unexpected name service %s.\n", name);
    } else {
        rp_ept = metal_allocate_memory(sizeof(struct rpmsg_endpoint));
        (void)rpmsg_create_ept(rp_ept, rdev, svc_name,
                       APP_EPT_ADDR, dest,
                       rpmsg_endpoint_cb,
                       rpmsg_service_unbind);
    }
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
    (void)ept;
    
    rpmsg_destroy_ept(rp_ept);
    LPRINTF("echo test: service is destroyed\n");
    metal_free_memory(rp_ept);
    rp_ept = NULL;
}

static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
    (void)src;
    (void)priv;
    int i;

    if(ept == rp_ept) {
       struct _payload *r_payload = (struct _payload *)data;

       LPRINTF(" received payload number %lu of size %lu \r\n",
           r_payload->num, len);

       if (r_payload->size == 0) {
       LPERROR(" Invalid size of package is received.\n");
       err_cnt++;
       return RPMSG_ERR_PARAM;
       }
       
       /* Validate data buffer integrity. */
       for (i = 0; i < (int)r_payload->size; i++) {
       if (r_payload->data[i] != 0xA5) {
          LPRINTF("Data corruption at index %d\n", i);
          err_cnt++;
          break;
       }
       }
       rnum = r_payload->num + 1;
    }

    return RPMSG_SUCCESS;
}

static int payload_init(struct rpmsg_device *rdev, struct payload_info *pi) {
    int rpmsg_buf_size = 0;

    /* Get the maximum buffer size of a rpmsg packet */
    if ((rpmsg_buf_size = rpmsg_virtio_get_buffer_size(rdev)) <= 0) {
        return rpmsg_buf_size;
    }

    pi->min = 1;
    pi->max = rpmsg_buf_size - 24;
    pi->num = pi->max / pi->min;

    i_payload =
        (struct _payload *)metal_allocate_memory(2 * sizeof(unsigned long) +
                      pi->max);
    if (!i_payload) {
        LPERROR("memory allocation failed.\n");
        return -ENOMEM;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    void *platform;
    struct rpmsg_device *rpdev;
    unsigned long proc_id = 0;
    unsigned long rsc_id = 0;
    int ret = 0;

    /* Initialize HW system components */
    init_system();

    if (argc >= 2) {
        proc_id = strtoul(argv[1], NULL, 0);
        rsc_id = proc_id;
    }
    if (argc >= 3) {
        testno = strtoul(argv[2], NULL, 0);
    } else {
        testno = 0;
    }

    if (testno == SHMEM_TEST) {
        ret = sharedMemTest(proc_id);
    } else {
        /* Initialize platform */
        ret = platform_init(proc_id, rsc_id, &platform);
        if (ret) {
            LPERROR("Failed to initialize platform.\n");
            ret = -1;
        } else {
            rpdev = platform_create_rpmsg_vdev(platform, 0,
                              VIRTIO_DEV_MASTER,
                              NULL,
                              rpmsg_name_service_bind_cb);
            if (!rpdev) {
                LPERROR("Failed to create rpmsg virtio device.\n");
                ret = -1;
            } else {
                app(rpdev, platform, proc_id);
                platform_release_rpmsg_vdev(platform, rpdev);
                ret = 0;
            }
        }

        LPRINTF("Stopping application...\n");
        platform_cleanup(platform);
    }
    cleanup_system();

    return ret;
}

