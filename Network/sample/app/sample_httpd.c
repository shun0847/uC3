/***************************************************************************
    HTTPd Sample
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */

#if SAMPLE_ENA_HTTPd
#include "http_server.h"


#if (0 == SAMPLE_USE_GENSRC)	/* no use configurator */
/*******************************
     HTTP Contents
 *******************************/
#include "index_html.h"		/* index_html1 */

/*******************************
        CGI Script definition
 *******************************/
extern void sample_fnc(T_HTTP_SERVER* http);

/*******************************
    HTTP Content List
 *******************************/
T_HTTP_FILE const content_list[] = {
    {"/", "text/html", index_html1, sizeof(index_html1), NULL},
    {"/sample.cgi", "", NULL, 0U, sample_fnc},
    {"", NULL, NULL, 0U, NULL}};

/*******************************
    HTTP Server Task
 *******************************/
SID ID_SOC_SNTPC;
SID ID_SOC_HTTP1;
#ifdef IPV6_SUP
SID ID_SOC_HTTP2; /* for IPv6*/
#endif

ID  ID_CONTENTS_MPF;
T_HTTP_SERVER       net_ctl_https;
const T_CMPF c_contents_mpf = {TA_TFIFO, 2, 256, NULL, NULL};
#if SAMPLE_ENA_WEBSOCKETd
extern ER ws_server_callback(T_HTTP_SERVER *sess);
#endif

void httpd_tsk(VP_INT exinf)
{
    (void)net_memset(&net_ctl_https, 0, sizeof(net_ctl_https));

    gHTTP_FILE             = (T_HTTP_FILE*)content_list;
    net_ctl_https.ver      = IP_VER4;
    net_ctl_https.SocketID = ID_SOC_HTTP1;

#ifdef HTTPD_SSL_SUP
    cfg_soc(net_ctl_https.SocketID, SOC_PRT_LOCAL, (VP)443);
    net_ctl_https.flag = HTTPD_FLG_SSL;		/* HTTPS */
#endif

#if SAMPLE_ENA_WEBSOCKETd   
    net_ctl_https.upgcbk = ws_server_callback;
#endif    
    (void)http_server(&net_ctl_https);
}

#if EXEC_CPU_64BIT
const T_CTSK net_ctsk_https = {TA_HLNG | TA_ACT | TA_FPU, (VP_INT)0, (FP)httpd_tsk,   6,
                               0x1800,           0,         "HttpServerTask"};
#else
const T_CTSK net_ctsk_https = {TA_HLNG | TA_ACT | TA_FPU, (VP_INT)0, (FP)httpd_tsk,   6,
                               0x800,           0,         "HttpServerTask"};
#endif

#ifdef IPV6_SUP
static T_HTTP_SERVER http_server6;
void httpd_tsk2(VP_INT exinf)
{
    net_memset((char*)&http_server6, 0, sizeof(http_server6));
    /* Listen from IPv6 address */
    http_server6.ver = IP_VER6;
    http_server6.SocketID = ID_SOC_HTTP2;
    http_server(&http_server6);
}
#if EXEC_CPU_64BIT
const T_CTSK ctsk_httpd2 = {
    TA_HLNG | TA_ACT, (VP_INT)0, (FP)httpd_tsk2, 5, 0x1800, 0,
    "HttpServerTask2"};
#else
const T_CTSK ctsk_httpd2 = {
    TA_HLNG | TA_ACT, (VP_INT)0, (FP)httpd_tsk2, 5, 0x800, 0,
    "HttpServerTask2"};
#endif
#endif

ER sample_httpd_ini()
{
	ER ercd;
    T_NODE lo_host = {0};

    lo_host.num = SAMPLE_SOCDEV_SER;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;
    lo_host.port = 80U;

    ercd         = cre_soc(IP_PROTO_TCP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_HTTP1 = (SID)ercd;
    (void)cfg_soc((SID)ercd, SOC_TMO_SND, (VP)25000U);
    (void)cfg_soc((SID)ercd, SOC_TMO_RCV, (VP)25000U);
    (void)cfg_soc((SID)ercd, SOC_TMO_CON, (VP)25000U);
    (void)cfg_soc((SID)ercd, SOC_TMO_CLS, (VP)25000U);

#ifdef IPV6_SUP
    lo_host.ver = IP_VER6;
    ercd         = cre_soc(IP_PROTO_TCP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_HTTP2 = (SID)ercd;
    (void)cfg_soc((SID)ercd, SOC_TMO_SND, (VP)25000U);
    (void)cfg_soc((SID)ercd, SOC_TMO_RCV, (VP)25000U);
    (void)cfg_soc((SID)ercd, SOC_TMO_CON, (VP)25000U);
    (void)cfg_soc((SID)ercd, SOC_TMO_CLS, (VP)25000U);
#endif

    ercd = acre_mpf((T_CMPF*)&c_contents_mpf);
    if (0 >= ercd) {
        return ercd;
    }
    ID_CONTENTS_MPF = ercd;

    /* SNTPc */
    lo_host.num = SAMPLE_SOCDEV_CLI;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;
    lo_host.port = PORT_ANY;
    ercd = cre_soc(IP_PROTO_UDP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_SNTPC = ercd;
	
    /* Starting Server Task  */
    gHTTP_FILE = (T_HTTP_FILE*)content_list;
    ercd       = acre_tsk((T_CTSK*)&net_ctsk_https);
    if (ercd <= E_OK) {
        return ercd;
    }
    (void)act_tsk((ID)ercd);

#ifdef IPV6_SUP
    ercd       = acre_tsk((T_CTSK*)&ctsk_httpd2);
    if (ercd <= E_OK) {
        return ercd;
    }
    (void)act_tsk((ID)ercd);
#endif

    return E_OK;
}
#else

#if SAMPLE_ENA_SSL
/* Note: HTTPS setting (http->flag = HTTPD_FLG_SSL) not yet supported by configurator*/
#ifdef HTTPD_SSL_SUP
#define HTTPD_SSL_PORT      443

extern UB is_tls_server_sup();

void httpd_ini_ssl(T_HTTP_SERVER *http)
{
    http->flag = HTTPD_FLG_SSL;
    cfg_soc(http->SocketID, SOC_PRT_LOCAL, (VP)HTTPD_SSL_PORT);
}

/*******************************
        httpd_tsk1
 *******************************/
extern T_HTTP_SERVER http_server1;
void httpd_tsk1(VP_INT exinf)
{
	if (!is_tls_server_sup())	return;
	
    httpd_ini_ssl(&http_server1);
    http_server(&http_server1);
}

#else
/*******************************
        httpd_tsk1
 *******************************/
extern T_HTTP_SERVER http_server1;
void httpd_tsk1(VP_INT exinf)
{
    http_server(&http_server1);
}

#endif  /* #ifdef HTTPD_SSL_SUP */

ER sample_httpd_ini()
{
    /* Start HTTPd Task */
    return sta_tsk(ID_HTTPD_TSK1, 0);  	
}
#else

ER sample_httpd_ini()
{
	return E_OK;
}

#endif  /* #if SAMPLE_ENA_SSL */

#endif  /* #if (0 == SAMPLE_USE_GENSRC) */

#endif  /* #if SAMPLE_ENA_HTTPd */
