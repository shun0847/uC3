/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, Network
    Network Bridge - IEEE 802.1D Implementataion Configuration File

    Generated at 2018-06-07 19:22:24
 **************************************************************************/

#include "kernel.h"
#include "kernel_id.h"
#include "brg_cfg.h"

/* RSTP resouces */
#if 0
ID ID_BRIDGE_TSK = ID_BRG_TSK;
ID ID_BRIDGE_TIM = ID_BRG_TIM;
ID ID_BRIDGE_FLG = ID_BRG_FLG;
ID ID_BRIDGE_MBX = ID_BRG_MBX;
ID ID_BRIDGE_SEM = ID_BRG_SEM;
#else
ID ID_BRGPRE_TSK = ID_BRG_TSK;
ID ID_BRGPRE_TIM = ID_BRG_TIM;
ID ID_BRGPRE_FLG = ID_BRG_FLG;
ID ID_BRGPRE_MBX = ID_BRG_MBX;
ID ID_BRGPRE_SEM = ID_BRG_SEM;
#endif

ER rstp_setup(void)
{
    ER ercd;

    /* Initialize bridge module */
    ercd = brg_ini();
    if (ercd != E_OK) {
        return ercd;
    }

    /* Add port to the bridge */
    ercd = brg_add_port(1);
    if (ercd != E_OK) {
        return ercd;
    }

    ercd = brg_add_port(2);
    if (ercd != E_OK) {
        return ercd;
    }

    return ercd;
}
/* end */
