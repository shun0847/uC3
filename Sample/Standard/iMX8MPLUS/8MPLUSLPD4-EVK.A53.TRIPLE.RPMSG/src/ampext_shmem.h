/**
 * @file    ampext_shmem.h
 * @brief   Sheard Memory Lock/Unlock Library
 * @date    2018.01.26
 * @author  Copyright (c) 2017-2018, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2017.12.28) yokota
 *            Initial version.
 *          - rev 1.1 (2018.01.26) yokota
 *            Fix typo.
 ****************************************************************************
 */
#ifndef AMPEXT_SHMEM_H_
#define AMPEXT_SHMEM_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    SHMEM_OK = 0,
    SHMEM_NOBLK = -1,
    SHMEM_USED  = -2,
    SHMEM_NOMEM = -3,
    SHMEM_NOT_FOUND = -4,
    SHMEM_PARAM = -5,
    SHMEM_ESTATUS  = -6,
    SHMEM_DEVERR = -7,
} SHMEM_ER;

typedef void* shmem_handler_t;

typedef struct {
    uint8_t  name[32];
    uint64_t size;
    uint32_t owner;
    bool     used;
    bool     lock;
} ref_blkinf;


typedef struct {
    uint8_t  busname[32];
    uint8_t  blkname[32];
    uint8_t  ctlname[32];
    uint32_t owner_value;
    uint32_t flags;
} shmem_init_t;

#define SHMEM_INIT_ALL    (0U)
#define SHMEM_INIT_REMOTE (1U)
#define SHMEM_INIT_NONE   (2U)

/*
 *  Sheard Memory Init
 *
 *  @param[out] handler
 *  @param[in]  blkname
 *  @param[in]  flags
 */
extern SHMEM_ER ampext_shmem_init(shmem_handler_t* handler, shmem_init_t* initparam);

/*
 *  Sheard Memory DeInit
 *
 *  @param[out] handler
 */
extern SHMEM_ER ampext_shmem_deinit(shmem_handler_t handler);

/*
 *  Sheard Memory Add
 *
 *  @param[out] handler
 *  @param[in] handler
 *
 */
extern SHMEM_ER ampext_shmem_add(shmem_handler_t handler, uint8_t* name, uint32_t size);

/*
 *  Sheard Memory Delete
 *
 *  @param[out] handler
 *  @param[in] handler
 *
 */
extern SHMEM_ER ampext_shmem_delete(shmem_handler_t handler, uint8_t* name);

/*
 *  Sheard Memory Get
 *
 *  @param[in] handler
 *  @param[in] name
 *  @param[out] blkid
 *  @param[out] addr
 *
 */
extern SHMEM_ER ampext_shmem_get(shmem_handler_t handler, uint8_t* name, uint32_t* blkid, void** addr);

/*
 *  Sheard Memory Return
 *
 *  @param[in] handler
 *  @param[in] blkid
 *
 */
extern SHMEM_ER ampext_shmem_ret(shmem_handler_t handler, uint32_t blkid);

/*
 *  Sheard Memory Info Reference
 *
 *  @param[in] handler
 *  @param[in] blkid
 *  @param[in] refp
 *
 */
extern SHMEM_ER ampext_shmem_ref(shmem_handler_t handler, uint32_t blkid, ref_blkinf* refp);

/*
 *  Sheard Memory Block initalize
 *
 *  @param[out] handler
 *  @param[in]  blkid
 *
 */
extern SHMEM_ER ampext_shmem_blkini(shmem_handler_t handler, uint32_t blkid);

/*
 *  Sheard Memory lock
 *
 *  @param[in] handler
 *  @param[in] blkid
 *  @param[in] refp
 *
 */
extern SHMEM_ER ampext_shmem_lock(shmem_handler_t handler, uint32_t blkid);

/*
 *  Sheard Memory unlock
 *
 *  @param[in] handler
 *  @param[in] blkid
 *  @param[in] refp
 *
 */
extern SHMEM_ER ampext_shmem_unlock(shmem_handler_t handler, uint32_t blkid);

#ifdef __cplusplus
}
#endif

#endif /* AMPEXT_SHMEM_H_ */
