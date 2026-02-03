#include "kernel.h"

/* uNet3/BSD resources */
/* for external memory pool spec */
ID ID_MPF_1600;
ID ID_MPF_4096;
ID ID_MPF_8192;
const T_CMPF c_net_mpf_1600 = {TA_TFIFO, 32, 1600, NULL, NULL};
const T_CMPF c_net_mpf_4096 = {TA_TFIFO, 8,  4096, NULL, NULL};
const T_CMPF c_net_mpf_8192 = {TA_TFIFO, 4,  8192, NULL, NULL};
extern UW NET_BUF_SZ;
ER net_memini(void)
{
    ER ercd;

    ercd = acre_mpf((T_CMPF *)&c_net_mpf_1600);
    if (ercd < 0) {
        return ercd;
    }
    ID_MPF_1600 = ercd;

    ercd = acre_mpf((T_CMPF *)&c_net_mpf_4096);
    if (ercd < 0) {
        del_mpf(ID_MPF_1600);
        return ercd;
    }
    ID_MPF_4096 = ercd;

    ercd = acre_mpf((T_CMPF *)&c_net_mpf_8192);
    if (ercd < 0) {
        del_mpf(ID_MPF_1600);
        del_mpf(ID_MPF_4096);
        return ercd;
    }
    ID_MPF_8192 = ercd;
    NET_BUF_SZ = 8192;
    
    return E_OK;
}

ER net_memext(void)
{
    del_mpf(ID_MPF_1600);
    del_mpf(ID_MPF_4096);
    del_mpf(ID_MPF_8192);
    return E_OK;
}

ER net_memget(VP *adr, UINT len, TMO tmo, ID *id)
{
    if (len <= 1600) {
        *id = ID_MPF_1600;
    } else if (len <= 4096) {
        *id = ID_MPF_4096;
    } else if (len <= 8192) {
        *id = ID_MPF_8192;
    } else {
        *id = 0;
        return E_NOMEM;
    }
    return tget_mpf(*id, adr, tmo);
}

ER net_memret(VP adr, ID id)
{
    return rel_mpf(id, adr);
}
