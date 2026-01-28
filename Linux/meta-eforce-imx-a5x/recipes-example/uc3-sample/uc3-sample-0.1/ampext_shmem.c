/**
 * @file    ampext_shmem.c
 * @brief   Sheard Memory Lock/Unlock Library
 * @date    2025.02.12
 * @author  Copyright (c) 2017-2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2017.12.28) yokota
 *            Initial version.
 *          - rev 1.1 (2018.01.26) yokota
 *            Fix typo.
 *          - rev 1.2 (2018.02.06) yokota
 *            Fix init, deinit function.
 *          - rev 1.3 (2018.02.28) yokota
 *            change metal_memset_io to memset function.
 *          - rev 1.4 (2018.12.21) yokota
 *            change for multi channel.
 *          - rev 1.5 (2019.03.08) yokota
 *            fix logical block ctl bug, blkid over 32.
 *          - rev 1.6 (2019.03.11) yokota
 *            fix ampext_shmem_init, suffix LLU to ULL.
 *          - rev 1.7 (2019.03.14) yokota
 *            change ampext_spinlock_acquire use atomic_compare_exchange_strong.
 *          - rev 1.8 (2019.04.26) yokota
 *            fix spinlock function.
 *          - rev 2.0 (2019.09.20) nozaki
 *            fix init function & separate spinlock functions to another file.
 *          - rev 2.1 (2020.05.11) Imada
 *            Fixed the used flag check in ampext_shmem_ret().
 *          - rev 2.2 (2021.04.19) Imada
 *            Fixed C++test warnings.
 *            Fixed incorrect comparison.
 *            Fixed a parameter used for strncpy.
 *          - rev 2.3 (2021.05.25) yokota
 *            Fixed C++test flow errpr and warnings.
 *            Fixed ICSA-21-119-04 problem.
 *          - rev 2.4 (2025.02.12)
 *            Fixed C++test flow warnings.
 ****************************************************************************
 */
#include <stddef.h>
#include "ampext_shmem.h"
#include "ampext_shmem_spinlock.h"
#include "metal/device.h"
#include "metal/io.h"
#include "metal/alloc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct t_shmem {
    uint64_t next; /* struct t_shmem *next; */
    uint64_t size;
} T_SHMEM;

/**
 *  Sheard Memory Header
 *
 */
typedef struct _shmem_blk_hdr {
    uint8_t  hdr[4];
    struct ampext_spinlock lock;
    uint64_t head; /* void* head; */
    uint64_t free_list; /* T_SHMEM* free_list; */
    uint8_t  body[];
} shmem_blk_hdr;

/**
 *  Sheard Memory Block Infomation
 */
typedef struct _shmem_rgn {
    uint8_t  name[32];
    uint64_t offset;
    uint64_t size;
    uint32_t owner;
    struct ampext_spinlock lock;
} shmem_rgn;

/**
 *  Sheard Memory Control Block Header
 */
typedef struct _shmem_ctl_body {
    uint64_t next; /* void *next; */
    uint8_t  name[32];
    uint64_t p_address;
    uint64_t size;
    uint64_t used;
    uint64_t fblk;
    uint64_t lockinfo;
    uint64_t free_list; /* T_SHMEM* free_list; */
    struct ampext_spinlock glock;
    shmem_rgn region[64];
} shmem_ctl_body;

/**
 *  Sheard Memory Control Block Header
 *  (This structure for workarea and not memory mapped.)
 */
typedef struct _shmem_ctl {
    uint8_t name[32];
    uint64_t ctl_offset;
    uint32_t owner_val;
    uint32_t initflags;
    struct metal_device *dev_blk;
    struct metal_device *dev_ctl;
    struct metal_io_region *io_blk;
    struct metal_io_region *io_ctl;
    ampext_spinlock_global_handler hlock;
} shmem_ctl;

#define MIN_BLKID  (1U)
#define MAX_BLKID  (64U)

#define SMEM_HDR   ((uint32_t)((uint32_t)'M' << 24) | ((uint32_t)'E' << 16) | ((uint32_t)'M' << 8) | (uint32_t)'S')

uint64_t getsize_shmemctl_body(void) {
    return sizeof(shmem_ctl_body);
}

uint64_t getsize_shmem_blk_hdr(void) {
    return sizeof(shmem_blk_hdr);
}

/***************************************
    Release Dynamic Memory Block
 ***************************************/
#define SHMEM_ALIGN_SIZE (8ULL)
#define SHMEM_MAX_SIZE   (0xFFFFFFFFFFFFFFFFULL)

static SHMEM_ER _shmem_relmem(struct metal_io_region *io, uint64_t* base, uint64_t blk, uint64_t sz)
{
    T_SHMEM *blk1, *blkn, *blkt;
    uint64_t nexta;
    bool loopend;
    SHMEM_ER ercd = SHMEM_OK;

    if (sz > (SHMEM_MAX_SIZE - SHMEM_ALIGN_SIZE)) {
        return SHMEM_PARAM;
    } else {
        sz = (sz + (SHMEM_ALIGN_SIZE - 1ULL)) & (uint64_t)(0LL - (int64_t)SHMEM_ALIGN_SIZE);
    }

    blkt = (T_SHMEM*)((uintptr_t)blk);
    blkt->next = 0ULL;
    blkt->size = sz;
    nexta = (uint64_t)metal_io_virt_to_phys(io, blkt) + sz;

    for(loopend = false; loopend == false; ) {
        blk1 = (T_SHMEM*)metal_io_phys_to_virt(io, *base);
        if (blk1 != 0) {
            if ((uint64_t)metal_io_virt_to_phys(io, blkt) < (uint64_t)metal_io_virt_to_phys(io, blk1)) {
                if (nexta < (uint64_t)metal_io_virt_to_phys(io, blk1)) {
                    blkt->next = (uint64_t)metal_io_virt_to_phys(io, blk1);
                    *base = (uint64_t)metal_io_virt_to_phys(io, blkt);
                } else if (nexta == (uint64_t)metal_io_virt_to_phys(io, blk1)) {
                    blkt->next = blk1->next;
                    blkt->size += blk1->size;
                    *base = (uint64_t)metal_io_virt_to_phys(io, blkt);
                } else {
                    ercd = SHMEM_PARAM;
                }
                loopend = true;
            } else if ((uint64_t)metal_io_virt_to_phys(io, blkt) == ((uint64_t)metal_io_virt_to_phys(io, blk1) + blk1->size)) {
                blk1->size += blkt->size;
                if (nexta == (uint64_t)blk1->next) {
                    blkn = (T_SHMEM*)metal_io_phys_to_virt(io, blk1->next);
                    if (blkn != 0) {
                        blk1->size += blkn->size;
                        blk1->next = blkn->next;
                    } else {
                        ercd = SHMEM_NOBLK;
                    }
                }
                loopend = true;
            } else {
                base = (uint64_t*)blk1;
            }
        } else {
            *base = metal_io_virt_to_phys(io, blkt);
            loopend = true;
        }
    }
    return ercd;
}

/***************************************
    Acurire Dynamic Memory Block
 ***************************************/
static T_SHMEM *_shmem_getmem(struct metal_io_region *io, uint64_t *base, uint64_t sz)
{
    T_SHMEM *blk;
    T_SHMEM *base_v;

    if (sz > (SHMEM_MAX_SIZE - SHMEM_ALIGN_SIZE)) {
        return NULL;
    } else {
        sz = (sz + (SHMEM_ALIGN_SIZE - 1ULL)) & (uint64_t)(0LL - (int64_t)SHMEM_ALIGN_SIZE);
    }

    for (blk = metal_io_phys_to_virt(io, *base); blk != 0; blk = metal_io_phys_to_virt(io, *base)) {
        if (blk->size >= sz) {
            if (blk->size == sz) {
                *base = (uint64_t)blk->next;
            } else if (blk->size > sz) {
                base_v = (T_SHMEM*)((uint8_t *)blk + sz);
                *base = (uint64_t)metal_io_virt_to_phys(io, base_v);
                base_v->size = blk->size - sz;
                base_v->next = blk->next;
            } else { 
                ; /* Do Nothing */
            }
            break;
        }
        base = (uint64_t*)blk;
    }
    return blk;
}

/**
 * @fn
 *   SHMEM_ER ampext_shmem_init(shmem_handler_t* handler, shmem_init_t* initparam)
 * @brief deinitalize sheard memory
 * @param[in] handler
 * @param[in] initparam initalize paramter
 * @retval SHMEM_OK       Success
 * @retval SHMEM_DEVERR   Device Error
 * @retval SHMEM_NOMEM    No memory region Error
 * @retval SHMEM_NOBLK    No (or invalid) block region Error
 * @retval SHMEM_PARAM    Parameter Error
 * @retval SHMEM_ESTATUS  Illegal Status
 */
SHMEM_ER ampext_shmem_init(shmem_handler_t* handler, shmem_init_t* initparam)
{
    ampext_spinlock_global_handler hlock;
    struct metal_device *dev;
    struct metal_io_region *io;
    static struct metal_io_region *metal_io_ctl;
    static struct metal_device *metal_dev_ctl;
    SHMEM_ER ercd = SHMEM_OK;
    shmem_ctl_body* ctl;
    shmem_ctl *instance;
    T_SHMEM *blk1;
    uint64_t p_addr;
    int ret;

    if ((handler == NULL) || (initparam == NULL)) {
        return SHMEM_PARAM;
    }
    if ((initparam->flags != SHMEM_INIT_ALL) && (initparam->flags != SHMEM_INIT_REMOTE) && (initparam->flags != SHMEM_INIT_NONE)) {
        return SHMEM_PARAM;
    }

    instance = metal_allocate_memory(sizeof(shmem_ctl));

    if (instance == NULL) {
        return SHMEM_NOMEM;
    }

    hlock = ampext_spinlock_global_init();
    if (hlock == NULL) {
        return SHMEM_DEVERR;
    }
    instance->hlock = hlock;

    ret = metal_device_open((char*)initparam->busname, (char*)initparam->ctlname, &dev);
    if (ret) {
        ercd = SHMEM_DEVERR;
        goto init_error;
    }
    io = metal_device_io_region(dev, 0x0U);
    if (!io) {
        ercd = SHMEM_NOMEM;
        goto init_error;
    }
    metal_io_ctl = io;
    metal_dev_ctl = dev;

    ret = metal_device_open((char*)initparam->busname, (char*)initparam->blkname, &dev);
    if (ret) {
        ercd = SHMEM_DEVERR;
        goto init_error;
    }
    io = metal_device_io_region(dev, 0x0U);
    if (!io) {
        ercd = SHMEM_NOMEM;
        goto init_error;
    }

    strcpy((char*)instance->name, (char*)initparam->blkname);
    instance->dev_ctl = metal_dev_ctl;
    instance->io_ctl = metal_io_ctl;
    instance->dev_blk = dev;
    instance->io_blk = io;
    instance->owner_val = initparam->owner_value;
    instance->initflags = initparam->flags;

    /* if not found SMEM HDR force SHMEM_INIT_ALL*/
    if (metal_io_read32(instance->io_ctl, 0) != SMEM_HDR) {
        initparam->flags = SHMEM_INIT_ALL;
        /* Clear SMEM Header Area */
        memset(metal_io_virt(instance->io_ctl, 0x0UL), 0, instance->io_ctl->size);
        shmem_blk_hdr* hdr = metal_io_virt(instance->io_ctl, 0x0U);
        if (hdr) {
            hdr->hdr[0] = 'S';
            hdr->hdr[1] = 'M';
            hdr->hdr[2] = 'E';
            hdr->hdr[3] = 'M';
            ampext_spinlock_init(&hdr->lock);
            ampext_spinlock_acquire(hlock, &hdr->lock, instance->owner_val);
            hdr->free_list = (uint64_t)metal_io_virt_to_phys(instance->io_ctl, &hdr->body[0]);
            blk1 = (T_SHMEM*)&hdr->body[0];
            blk1->next = 0ULL;
            blk1->size = (uint64_t)instance->io_ctl->size - (uint64_t)sizeof(shmem_blk_hdr);
            ctl =  (shmem_ctl_body*)_shmem_getmem(instance->io_ctl, &hdr->free_list, sizeof(shmem_ctl_body));
            hdr->head = (uint64_t)metal_io_virt_to_phys(instance->io_ctl, ctl);
            ampext_spinlock_release(hlock, &hdr->lock);
            instance->ctl_offset = metal_io_virt_to_offset(instance->io_ctl, ctl);
        } else {
            ercd = SHMEM_ESTATUS;
        }
    } else {
        p_addr = metal_io_read64(instance->io_ctl, offsetof(shmem_blk_hdr, head));
        /* serach physical block name */
        while (p_addr != 0U) {
            ctl = (shmem_ctl_body*)metal_io_phys_to_virt(instance->io_ctl, p_addr);
            if (ctl) {
                instance->ctl_offset = metal_io_virt_to_offset(instance->io_ctl, ctl);
                if (strncmp((char*)instance->name, (char*)ctl->name, 32U) == 0) {
                    /* found */
                    break;
                }
                p_addr = metal_io_read64(instance->io_ctl, instance->ctl_offset);
            } else {
                ercd = SHMEM_NOBLK;
                break;
            }
        }
        /* not found  then allocate block */
        if (p_addr == 0ULL) {
            shmem_blk_hdr* hdr = metal_io_virt(instance->io_ctl, 0x0U);
            if (hdr) {
                ampext_spinlock_init(&hdr->lock);
                ampext_spinlock_acquire(hlock, &hdr->lock, instance->owner_val);
                ctl =  (shmem_ctl_body*)_shmem_getmem(instance->io_ctl, &hdr->free_list, sizeof(shmem_ctl_body));
                if (ctl != NULL) {
                    metal_io_write64(instance->io_ctl, instance->ctl_offset, metal_io_virt_to_phys(instance->io_ctl, ctl));
                    instance->ctl_offset = metal_io_virt_to_offset(instance->io_ctl, ctl);
                    memset(metal_io_virt(instance->io_ctl, instance->ctl_offset), 0, sizeof(shmem_ctl_body));
                } else {
                    ercd = SHMEM_NOMEM;
                }
                ampext_spinlock_release(hlock, &hdr->lock);
            } else {
                ercd = SHMEM_ESTATUS;
            }
        }
    }

    if (ercd) {
        goto init_error;
    }

    ampext_spinlock_init(&ctl->glock);

    if (initparam->flags == SHMEM_INIT_ALL) {
        ampext_spinlock_acquire(hlock, &ctl->glock, instance->owner_val);
        strncpy((char*)ctl->name, (char*)instance->name, 32U);
        ctl->name[31] = '\0';
        ctl->next      = 0x0000000000000000ULL;
        ctl->used      = 0x0000000000000000ULL;
        ctl->lockinfo  = 0x0000000000000000ULL;
        ctl->fblk      = 0xFFFFFFFFFFFFFFFFULL;
        memset(metal_io_virt(instance->io_blk, 0x0U), 0, instance->io_blk->size);
        ctl->free_list = (uint64_t)metal_io_phys(instance->io_blk, 0x0U);
        blk1 = (T_SHMEM *)metal_io_virt(instance->io_blk, 0x0U);
        if (blk1) {
            blk1->next = 0ULL;
            blk1->size = instance->io_blk->size;

            for (int i = 0; i < 64; i++) {
                ctl->region[i].offset = 0ULL;
                ctl->region[i].size   = 0ULL;
                ctl->region[i].owner  = 0U;
                ampext_spinlock_init(&ctl->region[i].lock);
            }
        } else {
            ampext_spinlock_release(hlock, &ctl->glock);
            return SHMEM_NOBLK;
        }
        ampext_spinlock_release(hlock, &ctl->glock);
    } else if (initparam->flags == SHMEM_INIT_REMOTE) {
        ampext_spinlock_acquire(hlock, &ctl->glock, instance->owner_val);
        for (int i = 0; i < 64; i++) {
            /* Check not Owner and then init. */
            if (ctl->region[i].owner != instance->owner_val) {
                uint64_t p = (uint64_t)((uintptr_t)metal_io_virt(instance->io_blk, ctl->region[i].offset));
                (void)_shmem_relmem(instance->io_blk, &ctl->free_list, (uint64_t)p, ctl->region[i].size);
                ctl->region[i].size   = 0ULL;
                ctl->region[i].offset = 0ULL;
                ctl->region[i].owner  = 0U;
                ctl->used &= ~(1ULL << i);
                ctl->lockinfo &= ~(1ULL << i);
                ctl->fblk |= (1ULL << i);
                ampext_spinlock_init(&ctl->region[i].lock);
            }
        }
        ampext_spinlock_release(hlock, &ctl->glock);
    } else if (initparam->flags == SHMEM_INIT_NONE) {
        /* Nothing to do. */
        ;
    } else {
        return SHMEM_PARAM;
    }

    *handler = instance;
    return SHMEM_OK;

init_error:
    if (instance->dev_blk) {
        metal_device_close(instance->dev_blk);
    }
    if (instance->dev_ctl) {
        metal_device_close(instance->dev_ctl);
    }
    metal_free_memory(instance);
    *handler = NULL;
    return ercd;
}

/**
 * @fn
 *   SHMEM_ER ampext_shmem_deinit(shmem_handler_t handler)
 * @brief deinitalize sheard memory
 * @param[in] handler
 * @retval SHMEM_OK       Success
 * @retval SHMEM_PARAM    Parameter Error
 * @retval SHMEM_ESTATUS  Illegal Status
 */
SHMEM_ER ampext_shmem_deinit(shmem_handler_t handler)
{
    shmem_ctl *instance = (shmem_ctl *)handler;
    ampext_spinlock_global_handler hlock;
    shmem_blk_hdr* hdr;
    shmem_ctl_body* ctl;
    uint64_t phy_addr, boffset;

    if (instance == NULL) {
        return SHMEM_PARAM;
    }

    hlock = instance->hlock;
    if (hlock == NULL) {
        return SHMEM_ESTATUS;
    }

    hdr = metal_io_virt(instance->io_ctl, 0x0U);
    if (hdr) {
        if (metal_io_read32(instance->io_ctl, 0) != SMEM_HDR) {
            return SHMEM_ESTATUS;
        }
        if (instance->initflags == SHMEM_INIT_NONE) {
            memset(metal_io_virt(instance->io_blk, 0x0U), 0, instance->io_blk->size);
            metal_device_close(instance->dev_blk);
            ampext_spinlock_global_deinit(instance->hlock);
            metal_free_memory(instance);
            return SHMEM_OK;
        }

        boffset = offsetof(shmem_blk_hdr, head);
        ampext_spinlock_acquire(hlock, &hdr->lock, instance->owner_val);
        phy_addr = metal_io_read64(instance->io_ctl, boffset);
        while (phy_addr != 0ULL) {
            if (instance->ctl_offset == (phy_addr - (uint64_t)metal_io_phys(instance->io_ctl, 0x0U))) {
                metal_io_write64(instance->io_ctl, boffset, metal_io_read64(instance->io_ctl, instance->ctl_offset));
                break;
            }
            boffset = phy_addr - (uint64_t)metal_io_phys(instance->io_ctl, 0x0U);
            phy_addr  = metal_io_read64(instance->io_ctl, boffset);
        }
        if (phy_addr != 0ULL) {
            ctl = (shmem_ctl_body*)metal_io_virt(instance->io_ctl, instance->ctl_offset);
            (void)_shmem_relmem(instance->io_ctl, &hdr->free_list, (uint64_t)((uintptr_t)ctl), sizeof(shmem_ctl_body));
            memset(metal_io_virt(instance->io_blk, 0x0U), 0, instance->io_blk->size);
            metal_device_close(instance->dev_blk);
            ampext_spinlock_release(hlock, &hdr->lock);
            if (metal_io_read64(instance->io_ctl, offsetof(shmem_blk_hdr, head)) == 0ULL) {
                /* Last Block then close ctl block */
                memset(metal_io_virt(instance->io_ctl, 0x0U), 0, instance->io_ctl->size);
                metal_device_close(instance->dev_ctl);
            }
            ampext_spinlock_global_deinit(instance->hlock);
            metal_free_memory(instance);
        }
        return SHMEM_OK;
    } else {
        return SHMEM_ESTATUS;
    }
}

/**
 * @fn
 *   SHMEM_ER ampext_shmem_add(shmem_handler_t handler, uint8_t* name, uint32_t size)
 * @brief add memory block
 * @param[in] handler
 * @param[in] name
 * @retval SHMEM_OK      Success
 * @retval SHMEM_ESTATUS Illegal Status
 * @retval SHMEM_NOMEM   No (or invalid) memory region Error
 * @retval SHMEM_PARAM   Parameter Error
 * @retval SHMEM_NOT_FOUND  Shared memory not found Error
 */
SHMEM_ER ampext_shmem_add(shmem_handler_t handler, uint8_t* name, uint32_t size)
{
    shmem_ctl *instance = (shmem_ctl *)handler;
    ampext_spinlock_global_handler hlock;
    shmem_ctl_body* ctl;
    void *p;

    if (instance == NULL) {
        return SHMEM_PARAM;
    }

    hlock = instance->hlock;
    if (hlock == NULL) {
        return SHMEM_ESTATUS;
    }

    if (name == NULL) {
        return SHMEM_PARAM;
    }

    ctl = (shmem_ctl_body*)metal_io_virt(instance->io_ctl, instance->ctl_offset);
    if (ctl) {
        /* already registerd name check. */
        for (int i = 0; i < 64; i++) {
            if ((ctl->fblk & (1ULL << i)) == 0) {
                if (strncmp((char*)ctl->region[i].name, (char*)name, 32U) == 0) {
                    return SHMEM_PARAM;
                }
            }
        }

        for (int i = 0; i < 64; i++) {
            if ((ctl->fblk & (1ULL << i)) != 0) {
                ampext_spinlock_acquire(hlock, &ctl->glock, instance->owner_val);
                strncpy((char*)ctl->region[i].name, (char*)name, 32U);
                ctl->region[i].name[31] = '\0';
                p = (void*)_shmem_getmem(instance->io_blk, &ctl->free_list, (uint64_t)size);
                if (p == NULL)
                    return SHMEM_NOMEM;
                ctl->fblk &= ~(1ULL << i);
                ctl->region[i].size   = size;
                ctl->region[i].offset = metal_io_virt_to_offset(instance->io_blk, p);
                ctl->region[i].owner  = instance->owner_val;
                ampext_spinlock_release(hlock, &ctl->glock);
                return SHMEM_OK;
            }
        }
        return SHMEM_NOT_FOUND;
    } else {
        return SHMEM_ESTATUS;
    }
}

/**
 * @fn
 *   SHMEM_ER ampext_shmem_delete(shmem_handler_t handler, uint8_t* name)
 * @brief return memory block, and change unused status
 * @param[in] handler
 * @param[in] name
 * @retval SHMEM_OK      Success
 * @retval SHMEM_PARAM   Parameter Error
 * @retval SHMEM_ESTATUS Illegal Status
 * @retval SHMEM_NOT_FOUND  Shared memory not found Error
 */
SHMEM_ER ampext_shmem_delete(shmem_handler_t handler, uint8_t* name)
{
    shmem_ctl *instance = (shmem_ctl *)handler;
    ampext_spinlock_global_handler hlock;
    shmem_ctl_body* ctl;
    void* p;

    if (instance == NULL) {
        return SHMEM_PARAM;
    }

    hlock = instance->hlock;
    if (hlock == NULL) {
        return SHMEM_ESTATUS;
    }

    if (name == NULL) {
        return SHMEM_PARAM;
    }

    ctl = (shmem_ctl_body*)metal_io_virt(instance->io_ctl, instance->ctl_offset);
    if (ctl) {
        for (int i = 0; i < 64; i++) {
            if (strncmp((char*)ctl->region[i].name, (char*)name, 32U) == 0) {
                ampext_spinlock_acquire(hlock, &ctl->glock, instance->owner_val);
                ampext_spinlock_init(&ctl->region[i].lock);
                memset(ctl->region[i].name, 0, 32U);
                ctl->fblk |= (1ULL << i);
                ctl->used &= ~(1ULL << i);
                ctl->lockinfo &= ~(1ULL << i);
                p = metal_io_virt(instance->io_blk, ctl->region[i].offset);
                (void)_shmem_relmem(instance->io_blk, &ctl->free_list, (uint64_t)((uintptr_t)p),  ctl->region[i].size);
                ctl->region[i].offset = 0ULL;
                ctl->region[i].size   = 0ULL;
                ctl->region[i].owner  = 0U;
                ampext_spinlock_release(hlock, &ctl->glock);
                return SHMEM_OK;
            }
        }
        return SHMEM_NOT_FOUND;
    } else {
        return SHMEM_ESTATUS;
    }
}

/**
 * @fn
 *   SHMEM_ER ampext_shmem_ref(shmem_handler_t handler, uint32_t blkid, ref_blkinf* refp)
 * @brief return memory block infomation.
 * @param[in]  handler
 * @param[in]  blkid
 * @param[out] refp
 * @retval SHMEM_OK      Success
 * @retval SHMEM_PARAM   Parameter Error
 * @retval SHMEM_ESTATUS Illigal Status
 */
SHMEM_ER ampext_shmem_ref(shmem_handler_t handler, uint32_t blkid, ref_blkinf* refp)
{
    shmem_ctl *instance = (shmem_ctl *)handler;
    shmem_ctl_body* ctl;

    if (instance == NULL) {
        return SHMEM_PARAM;
    }

    /* 1 <= blkid <= 64, but region[0] - region[63] are valid */
    if ((blkid < MIN_BLKID) || (blkid > MAX_BLKID)) {
        return SHMEM_PARAM;
    }

    if (refp == NULL) {
        return SHMEM_PARAM;
    }

    ctl = (shmem_ctl_body*)metal_io_virt(instance->io_ctl, instance->ctl_offset);
    if (ctl) {
        blkid -= MIN_BLKID;
        if ((ctl->fblk & (1ULL << blkid)) != 0U) {
            return SHMEM_ESTATUS;
        }

        strncpy((char*)refp->name, (char*)ctl->region[blkid].name, 32U);
        refp->name[31] = '\0';
        refp->size  = ctl->region[blkid].size;
        refp->owner = ctl->region[blkid].owner;
        refp->lock = (ctl->lockinfo & (1ULL << blkid))?(bool)true:(bool)false;
        refp->used = (ctl->used & (1ULL << blkid))?(bool)true:(bool)false;
        return SHMEM_OK;
    } else {
        return SHMEM_ESTATUS;
    }
}

/**
 * @fn
 *   SHMEM_ER ampext_shmem_get(shmem_handler_t handler, uint8_t* name, uint32_t* blkid, void** addr)
 * @brief get memory infomation
 * @param[in] handler
 * @param[in] blkid
 * @retval SHMEM_OK      Success
 * @retval SHMEM_PARAM   Parameter Error
 * @retval SHMEM_USED    Shared memory already used Error
 * @retval SHMEM_NOT_FOUND  Shared memory not found Error
 * @retval SHMEM_ESTATUS Illigal Status
 */
SHMEM_ER ampext_shmem_get(shmem_handler_t handler, uint8_t* name, uint32_t* blkid, void** addr)
{
    shmem_ctl *instance = (shmem_ctl *)handler;
    ampext_spinlock_global_handler hlock;
    shmem_ctl_body* ctl;

    if (instance == NULL) {
        return SHMEM_PARAM;
    }

    hlock = instance->hlock;
    if (hlock == NULL) {
        return SHMEM_ESTATUS;
    }

    if ((blkid == NULL) || (name == NULL) || (addr == NULL)) {
        return SHMEM_PARAM;
    }

    ctl = (shmem_ctl_body*)metal_io_virt(instance->io_ctl, instance->ctl_offset);
    if (ctl) {
        for (int i = 0; i < 64; i++) {
            if (strncmp((char*)ctl->region[i].name, (char*)name, 32U) == 0) {
                *blkid = ((uint32_t)i+MIN_BLKID);
                *addr  = (void*)metal_io_virt(instance->io_blk, ctl->region[i].offset);
                if ((ctl->used & (1ULL << i)) != 0) {
                    return SHMEM_USED;
                }
                else {
                    ampext_spinlock_acquire(hlock, &ctl->glock, instance->owner_val);
                    ctl->used |= (1ULL << i);
                    ampext_spinlock_release(hlock, &ctl->glock);
                    return SHMEM_OK;
                }
            }
        }
        return SHMEM_NOT_FOUND;
    } else {
        return SHMEM_ESTATUS;
    }
}

/**
 * @fn
 *   SHMEM_ER ampext_shmem_blkini(shmem_handler_t handler, uint32_t blkid)
 * @brief initalize memory block and status
 * @param[in] handler
 * @param[in] blkid
 * @retval SHMEM_OK      Success
 * @retval SHMEM_PARAM   Parameter Error
 * @retval SHMEM_ESTATUS Illigal Status
 */
SHMEM_ER ampext_shmem_blkini(shmem_handler_t handler, uint32_t blkid)
{
    shmem_ctl *instance = (shmem_ctl *)handler;
    ampext_spinlock_global_handler hlock;
    shmem_ctl_body* ctl;

    if (instance == NULL) {
        return SHMEM_PARAM;
    }

    hlock = instance->hlock;
    if (hlock == NULL) {
        return SHMEM_ESTATUS;
    }

    /* 1 <= blkid <= 64, but region[0] - region[63] are valid */
    if ((blkid < MIN_BLKID) || (blkid > MAX_BLKID)) {
        return SHMEM_PARAM;
    }

    ctl = (shmem_ctl_body*)metal_io_virt(instance->io_ctl, instance->ctl_offset);
    if (ctl) {
        blkid -= MIN_BLKID;

        if ((ctl->fblk & (1ULL << blkid)) != 0U) {
            return SHMEM_ESTATUS;
        }

        ampext_spinlock_acquire(hlock, &ctl->glock, instance->owner_val);
        ctl->used &= ~(1ULL << blkid);
        ctl->lockinfo &= ~(1ULL << blkid);
        ampext_spinlock_init(&ctl->region[blkid].lock);
        ampext_spinlock_acquire(hlock, &ctl->region[blkid].lock, instance->owner_val);
        ctl->lockinfo |= (1ULL << blkid);
        memset(metal_io_virt(instance->io_blk, ctl->region[blkid].offset), 0, (size_t)ctl->region[blkid].size);
        ampext_spinlock_release(hlock, &ctl->region[blkid].lock);
        ctl->lockinfo &= ~(1ULL << blkid);
        ampext_spinlock_init(&ctl->region[blkid].lock);
        ampext_spinlock_release(hlock, &ctl->glock);
        return SHMEM_OK;
    } else {
        return SHMEM_ESTATUS;
    }
}


/**
 * @fn
 *   SHMEM_ER ampext_shmem_ret(shmem_handler_t handler, uint32_t blkid)
 * @brief return memory block, and change unused status
 * @param[in] handler
 * @param[in] blkid
 * @retval SHMEM_OK      Success
 * @retval SHMEM_PARAM   Parameter Error
 * @retval SHMEM_ESTATUS Illigal Status
 */
SHMEM_ER ampext_shmem_ret(shmem_handler_t handler, uint32_t blkid)
{
    shmem_ctl *instance = (shmem_ctl *)handler;
    ampext_spinlock_global_handler hlock;
    shmem_ctl_body* ctl;

    if (instance == NULL) {
        return SHMEM_PARAM;
    }

    hlock = instance->hlock;
    if (hlock == NULL) {
        return SHMEM_ESTATUS;
    }

    /* 1 <= blkid <= 64, but region[0] - region[63] are valid */
    if ((blkid < MIN_BLKID) || (blkid > MAX_BLKID)) {
        return SHMEM_PARAM;
    }

    ctl = (shmem_ctl_body*)metal_io_virt(instance->io_ctl, instance->ctl_offset);
    if (ctl) {
        blkid -= MIN_BLKID;

        if ((ctl->used & (1ULL << blkid)) == 0U) {
            return SHMEM_ESTATUS;
        }

        ampext_spinlock_acquire(hlock, &ctl->glock, instance->owner_val);
        ctl->used &= ~(1ULL << blkid);
        ampext_spinlock_release(hlock, &ctl->glock);
        return SHMEM_OK;
    } else {
        return SHMEM_ESTATUS;
    }
}

/**
 * @fn
 *   SHMEM_ER ampext_shmem_lock(shmem_handler_t handler, uint32_t blkid)
 * @brief lock sheard memory
 * @param[in] handler
 * @param[in] blkid
 * @retval SHMEM_OK Success
 * @retval SHMEM_PARAM Parameter Error
 * @retval SHMEM_ESTATUS Illigal Status
 */
SHMEM_ER ampext_shmem_lock(shmem_handler_t handler, uint32_t blkid)
{
    shmem_ctl *instance = (shmem_ctl *)handler;
    ampext_spinlock_global_handler hlock;
    shmem_ctl_body* ctl;

    if (instance == NULL) {
        return SHMEM_PARAM;
    }

    hlock = instance->hlock;
    if (hlock == NULL) {
        return SHMEM_ESTATUS;
    }
    /* 1 <= blkid <= 64, but region[0] - region[63] are valid */
    if ((blkid < MIN_BLKID) || (blkid > MAX_BLKID)) {
        return SHMEM_PARAM;
    }
    ctl = (shmem_ctl_body*)metal_io_virt(instance->io_ctl, instance->ctl_offset);
    if (ctl) {
        blkid -= MIN_BLKID;
        if ((ctl->fblk & (1ULL << blkid)) != 0U) {
            return SHMEM_PARAM;
        }
        ampext_spinlock_acquire(hlock, &ctl->region[blkid].lock, instance->owner_val);
        ampext_spinlock_acquire(hlock, &ctl->glock, instance->owner_val);
        ctl->lockinfo |= (1ULL << blkid);
        ampext_spinlock_release(hlock, &ctl->glock);

        atomic_thread_fence(memory_order_acq_rel);
        return SHMEM_OK;
    } else {
        return SHMEM_ESTATUS;
    }
}

/**
 * @fn
 *   SHMEM_ER ampext_shmem_unlock(shmem_handler_t handler, uint32_t blkid)
 * @brief unlock sheard memory
 * @param[in] handler
 * @param[in] blkid
 * @retval SHMEM_OK Success
 * @retval SHMEM_PARAM Parameter Error
 * @retval SHMEM_ESTATUS Illigal Status
 */
SHMEM_ER ampext_shmem_unlock(shmem_handler_t handler, uint32_t blkid)
{
    shmem_ctl *instance = (shmem_ctl *)handler;
    ampext_spinlock_global_handler hlock;
    shmem_ctl_body* ctl;

    if (instance == NULL) {
        return SHMEM_PARAM;
    }

    hlock = instance->hlock;
    if (hlock == NULL) {
        return SHMEM_ESTATUS;
    }
    /* 1 <= blkid <= 64, but region[0] - region[63] are valid */
    if ((blkid < MIN_BLKID) || (blkid > MAX_BLKID)) {
        return SHMEM_PARAM;
    }
    ctl = (shmem_ctl_body*)metal_io_virt(instance->io_ctl, instance->ctl_offset);
    if (ctl) {
        blkid -= MIN_BLKID;
        if ((ctl->fblk & (1ULL << blkid)) != 0U) {
            return SHMEM_PARAM;
        }
        ampext_spinlock_acquire(hlock, &ctl->glock, instance->owner_val);
        ctl->lockinfo &= ~(1ULL << blkid);
        ampext_spinlock_release(hlock, &ctl->glock);
        ampext_spinlock_release(hlock, &ctl->region[blkid].lock);
        atomic_thread_fence(memory_order_acq_rel);
        return SHMEM_OK;
    } else {
        return SHMEM_ESTATUS;
    }
}

#ifdef __cplusplus
}
#endif

