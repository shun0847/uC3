/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    HTTP Server extention (Multipart receive CGI)
    Copyright (c)  2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.03.30: Created
 ***************************************************************************/

#ifndef HTTPD_MP_CGI_H
#define HTTPD_MP_CGI_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "http_server.h"
#include "net_strlib.h"

/*- STRUCTURE   ----------------------------------------------*/
/* multipart header information for httpd_mp_hdr_info() */
typedef struct t_http_mphdr {
    VB *disp;
    VB *name;
    VB *file;
    VB *ctype;
} T_HTTP_MPHDR;

/* multipart notify information for User Callback */
typedef struct t_http_mpcgi {
    T_HTTP_SERVER *http;
    VP dat;
    VP user;
    UH len;
    UB stat;
} T_HTTP_MPCGI;


/*- DEFINES --------------------------------------------------*/
/* multipart notify status (T_HTTP_MPCGI::stat) */
#define MPCGI_STAT_START        0U      /* start CGI */
#define MPCGI_STAT_END          1U      /* end CGI (include error) */

#define MPCGI_STAT_MPHEAD       3U      /* read multipart header */
#define MPCGI_STAT_MPBODY       4U      /* read multipart body */
#define MPCGI_STAT_MPBODYEND    5U      /* read multipart body terminate */

/* multipart boundary check result for httpd_mp_bdry_chk */
#define MPBDRY_CHK_NOFOUND      0x01U   /* not found header/end */
#define MPBDRY_CHK_FOUND        0x02U   /* found header/end */
#define MPBDRY_CHK_PART         0x04U   /* found part of header/end */
#define MPBDRY_CHK_ERROR        0x08U   /* process error */
#define MPBDRY_POS_HEADER       0x10U   /* multipart header */
#define MPBDRY_POS_END          0x20U   /* multipart end */


/*- FUNCTIONS   ----------------------------------------------*/
ER httpd_mp_cgi(T_HTTP_SERVER *http, const T_HTTP_FILE *fp);

ER httpd_mp_valid(VB *hdr_ctype, VB *boundary);     /* is valid support multipart ? */
ER httpd_mp_setup(T_HTTP_SERVER *http);           /* streaming receive CGI setup */
ER httpd_mp_content_rcv(T_HTTP_SERVER *http);       /* receive contents data */
VB* httpd_mp_bdry_chk(VB *str, UH len, const VB *boundary, UB *ret);    /* boundary check */
ER httpd_mp_hdr_info(T_HTTP_MPHDR *mp_hdr, VB* str, UH len);          /* parse multipart header */


#ifdef __cplusplus
}
#endif
#endif /* HTTPD_MP_CGI_H */
