/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, Network
    Network Bridge - IEEE 802.1D Implementataion
    Copyright (c) 2015, eForce Co., Ltd. All rights reserved.

    Version Information
      2015.09.28: Created
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "brg_mng.h"
#include "DDR_KSZ8863.h"

void hal_flush_db(UH portNum)
{
    // KSZ8863 does not support flush per port
    // flush for all ports
    ksz8863_flush_dynamic();
}

void hal_discard_port(UH portNum)
{
    ksz8863_disc_port(portNum);
}

void hal_forward_port(UH portNum)
{
    ksz8863_foward_port(portNum);
}

void hal_learn_port(UH portNum)
{
    ksz8863_learn_port(portNum);
}

void hal_add_entry(UH portNum, UB *mac)
{
    ksz8863_add_entry(portNum, mac);
}

void hal_rmv_entry(UH portNum, UB *mac)
{
    ksz8863_rmv_entry(portNum, mac);
}

void hal_get_entry(UH portNum, UB *mac)
{
    // func not used
}

void hal_set_ageing(UH portNum, UH ageingTime)
{
    // feature not supported by KSZ8863
}

void hal_get_ageing(UH portNum, UH *ageingTime)
{
    // func not used
}

void hal_get_portState(UH portNum, UB *state)
{
    ksz8863_get_state(portNum, state);
}

void hal_get_pathCost(UH portNum, UW *cost)
{
    UH type = PHY_STS_100FD;

    ksz8863_get_type(portNum, &type);
    switch (type) {
        case PHY_STS_100FD:
        case PHY_STS_100HD:
            *cost = PATH_COST_100MB;
            break;
        case PHY_STS_10FD:
        case PHY_STS_10HD:
            *cost = PATH_COST_10MB;
            break;
        default:
            *cost = PATH_COST_100MB;
            break;
    }
}

void hal_get_mac(UH portNum, UB *mac)
{
    T_NET_DEV *dev;

    /* get a unique mac address for Bridge */
    dev = &gNET_DEV[0];
    memcpy(&mac[0], &dev->cfg.eth.mac[0], 6);    /* use device mac address */
}
