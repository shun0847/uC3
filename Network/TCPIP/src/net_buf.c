/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    Network buffer memory management functions.
    Copyright (c)  2008-2016, eForce Co., Ltd. All rights reserved.

    Version Information  2008.11.19: Created
                         2012.10.02  Modify to avoid use of string libraries.
                         2013.12.06  Change to external function to allocate buffer
                         2016.02.18  Change the second parameter type of net_buf_get
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"

/***************************************************************************
    Initializes the resources required for network buffer allocation.

 [API]          ER net_buf_ini(void)
 [Param     ]   None
 [Success   ]   E_OK
 [Error     ]
                E_xxx   Memory pool creation error.
****************************************************************************/

ER net_buf_ini(void)
{
    return net_memini();
}

/***************************************************************************
    Allocates memory for a network packet. Because of fixed memory pool is
    used, the actual allocated memory may be greater than the requested
    length.

 [API]          ER net_buf_get(T_NET_BUF **buf, UW len, TMO tmo)
 [Param     ]   **buf       Allocated buffer address pointer.
                len         Required buffer size.
                tmo         API blocks until this timeout if memory is not
                            available.
 [Success   ]   E_OK        *buf = buffer address.

 [Error     ]               *buf = NULL
                E_PAR       Invalid 'len'.
                E_TMO       Allocation process time out.
                E_NOMEM     Memory cannot be allocated.
****************************************************************************/

ER net_buf_get(T_NET_BUF **buf, UW len, TMO tmo)
{
    T_NET_BUF *pkt;
    ER ercd;
    ID id;

    if (len == 0) {
        return E_PAR;
    }

    *buf = NULL;
    ercd = net_memget((VP*)&pkt, len, tmo, &id);

    if (ercd == E_OK) {
        net_memset(pkt, 0, sizeof(T_NET_BUF));
        pkt->mpfid = id;
        pkt->hdr   = &pkt->buf[0];
        *buf = pkt;
    }

    return ercd;
}

/***************************************************************************
    Releases the memory of the network buffer. Also, the packet's socket
    process is invoked if any.

 [API]          void net_buf_ret(T_NET_BUF *buf)
 [Param     ]   *buf        Buffer address to be released.
 [Success   ]
 [Error     ]
****************************************************************************/

void net_buf_ret(T_NET_BUF *buf)
{
    if (buf == NULL) {
        return;
    }

    if (buf->soc) {
        soc_wakeup(buf);
    }

    net_memret(buf, buf->mpfid);
}


#ifndef EXT_ALOC_SUP

#ifndef NET_C_OS
ID ID_NET_MPF=0;
#endif

ER net_memini(void)
{
#ifndef NET_C_OS
    ER ercd;
    ercd = acre_mpf((T_CMPF *)&c_net_mpf);
    if (ercd < 0) {
        ID_NET_MPF = 0;
        return ercd;
    }
    ID_NET_MPF = ercd;
#endif
    return E_OK;
}

ER net_memext(void)
{
#ifndef NET_C_OS
    if (ID_NET_MPF > 0) {
        del_mpf(ID_NET_MPF);
    }
#endif
    return E_OK;
}

ER net_memget(VP *adr, UINT len, TMO tmo, ID *id)
{
    if (len > NET_BUF_SZ) {
        return E_NOMEM;
    }

    *id = ID_NET_MPF;
    return tget_mpf(*id, adr, tmo);
}

ER net_memret(VP adr, ID id)
{
    return rel_mpf(id, adr);
}

#endif /* EXT_ALOC_SUP */

/*EOF*/
