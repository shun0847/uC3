/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    WebSocket Client application
    Copyright (c)  2021-2023, eForce Co., Ltd. All rights reserved.

    Version Information
      2021.07.01: Created
      2022.02.15: fix parameter check
      2023.07.25: Supported WSS connection.
      2023.08.31: Fixed header addition error in ws_http_con().
      2025.07.30: Supported IPv6 communication using uNet3-IPv6.
 ***************************************************************************/
#include "kernel.h"
#include "net_hdr.h"
#ifdef NET_C_OS
#include "kernel_id.h"
#include "net_id.h"
#endif

#include "net_strlib.h"
#include "http_client.h"
#include "web_soc.h"

#define memset      net_memset
#define memcpy      net_memcpy
#define memcmp      net_memcmp
#define strcpy      net_strcpy
#define strlen      net_strlen
#define strncasecmp net_strncasecmp

extern void simple_sprintf(char *str, const char *fmt, ...);
#define sprintf     simple_sprintf



/* Enable WS_AUTOBAHN to use with Autobahn test suite*/
//#define WS_AUTOBAHN

/* Set Websocket server's address */
#define WS_SERVER_IP            "192.168.100.5"
#define WS_SERVER_PORT          "9001"
static UB wsuri[HOST_NAME_LEN];
static UW dns_addr = 0xC0A86401;    /* dns server address */

/*Application buffer for data received in single frame. Set it equal to WS_FRAME_MAX_PAYLOAD_SIZE */
#define WSS_APP_BUF_SIZE        WS_FRAME_MAX_PAYLOAD_SIZE

/*Application buffer for data received in fragments. Set it >= WS_FRAME_MAX_PAYLOAD_SIZE.
  Maximum size is depends on application*/
#define WSS_APP_FRG_BUF_SIZE    WS_FRAME_MAX_PAYLOAD_SIZE

static T_HTTP_CLIENT http;
static T_WS          g_cli_ws;
static UB ws_key[24] = {0};  /* size of nonce in bits / 6 and align for 4 byte boundary. */
static UB ws_hash[28] = {0}; /* calculated value for the given nonce */
static UB key_ws[25];

#ifdef ENA_CUSTOM_HEADER
/* Define static custom header in http_chdr_tbl[] */
const T_HTTP_CUSTOM_HDR http_chdr_tbl[] = {
    { NULL, NULL, NULL, NULL, 0 }   /* Terminate mark (Do not change) */
};
#endif

extern ER ws_gen_key(UB *key, UB *hash);
extern ER ws_srv_key(UB *key, UB *hash);

#ifdef NET_S_OS
extern SID ID_SOC_HTTPC;
extern ID ID_HTTP_MSG_MPF;
extern SID ID_SOC_DNS;
ID ID_WEB_SOCKET_MPF;
T_CMPF websocket_mpf = {TA_TFIFO, 10U, 24U, NULL, NULL};
#else
/* Note: When using ID_CHDR_MPF for WebSocket, set the number of blocks to 5 or more. */
#ifndef ID_WEB_SOCKET_MPF
#define ID_WEB_SOCKET_MPF   ID_CHDR_MPF
#endif
#endif

ER ws_http_con(T_HTTP_CLIENT *hc, UB *url)
{
    T_HTTP_CUSTOM_HDR *elem;
    ER ercd;
    UB comp_hash[28];

#ifdef NET_S_OS
    ercd = acre_mpf((T_CMPF *)&websocket_mpf);
    if (ercd < 0) {
        return ercd;
    }
    ID_WEB_SOCKET_MPF = ercd;
#endif

    ercd = E_OBJ;
    while(1) {
        /* Use HTTP custom headers API to include WebSocket headers in GET request */
        ercd = http_ehdr_ini(hc, ID_WEB_SOCKET_MPF);
        if (ercd != E_OK) {
            break;
        }

        /* Include Connection: upgrade header */
        ercd = http_ehdr_ins(hc, "Connection","Upgrade",CUSTM_HDR_DIR_REQ | CUSTM_HDR_DIR_RES);
        if (ercd != E_OK) {
            break;
        }

        /* Include Upgrade: WebSocket header */
        ercd = http_ehdr_ins(hc, "Upgrade","websocket",CUSTM_HDR_DIR_REQ | CUSTM_HDR_DIR_RES);
        if (ercd != E_OK) {
            break;
        }

        /* Generating Key for Encryption and verifying hash from server */
        ws_gen_key(ws_key,ws_hash);

        /* Include Sec-Websocket-key: <key value> */
        memcpy(key_ws, ws_key, sizeof(ws_key));
        key_ws[24] = '\0';
        ercd = http_ehdr_ins(hc, "Sec-Websocket-key", (VB*)key_ws ,CUSTM_HDR_DIR_REQ | CUSTM_HDR_DIR_RES);
        if (ercd != E_OK) {
            break;
        }

        /* Include Sec-Websocket-Version: 13 */
        ercd = http_ehdr_ins(hc, "Sec-Websocket-Version","13",CUSTM_HDR_DIR_REQ | CUSTM_HDR_DIR_RES);
        if (ercd != E_OK) {
            break;
        }

        ercd = http_ehdr_ins(hc, "Sec-WebSocket-Accept",NULL, CUSTM_HDR_DIR_RES);
        if (ercd != E_OK) {
            break;
        }

        ercd = E_OK;
        break;
    }

    if (ercd != E_OK) {
        if (E_TMOUT == ercd)    return E_NOMEM;
        (void)http_ehdr_end(hc);
        return ercd;
    }


    ercd = E_OBJ;
    while (1) {
        /* Send GET request and receive response */
        ercd = http_cmd_get(hc, (char*)url);
        if (ercd != E_OK) {
            break;
        }

        /* Check Websocket headers in GET response */

        /* Check for Connection header */
        elem = http_ehdr_get(hc, "Connection", CUSTM_HDR_DIR_RES);
        if ((elem ==NULL) || (elem !=NULL && (strncasecmp(elem->response, "upgrade", 7)!=0 ))) {
            ercd = E_OBJ;
            break;
        }

        /* Check for upgrade header */
        elem = http_ehdr_get(hc, "Upgrade", CUSTM_HDR_DIR_RES);
        if ((elem == NULL) || (elem != NULL && (strncasecmp(elem->response,"websocket",9) != 0))) {
                ercd = E_OBJ;
            break;
        }
            
        /* Check Version header(for debug) */
        elem = http_ehdr_get(hc, "Sec-Websocket-Version", CUSTM_HDR_DIR_RES);
        if (elem != NULL) {
            if ((elem->response != NULL) && (strncasecmp(elem->response,"13",2) != 0)) {
                ercd = E_OBJ;
                break;
            }
        }
    
        /* Check Accept header and validate hash */
        elem = http_ehdr_get(hc, "Sec-WebSocket-Accept", CUSTM_HDR_DIR_RES);
        ws_srv_key(key_ws, comp_hash);
        if (elem != NULL) {
            if (memcmp(comp_hash, elem->response , sizeof(comp_hash))!=0) {
                ercd = E_OBJ;
                break;
            }
        } else {
            ercd = E_OBJ;
            break;
        }

        ercd = E_OK; /* connection successful */
        break;
    }
    
    if (ercd != E_OK) {
        http_cls(hc);
        (void)http_ehdr_end(hc);
        return ercd;
    }

    /* Clear custom header. */
    ercd = http_ehdr_end(hc);
    if (ercd != E_OK) {
        http_cls(hc);
    }

    return ercd;
}

static ER ws_cli_con(UB *url)
{
    ER ercd;
    T_NODE nod;

    /* Initialize HTTP Client structure */
    memset(&http, 0, sizeof(http));
    http.mpf_id = ID_HTTP_MSG_MPF;
    http.http_sid = ID_SOC_HTTPC;

    (void)ref_soc(hc.http_sid, SOC_IP_LOCAL, (VP)&nod);

    /* Get IP address of HTTP Server */
    if (nod.ver == IP_VER4) {
    ercd = http_get_ipaddr((char const*)url, &http.svr, ID_SOC_DNS, dns_addr);
    } else {
        ercd = http_get_ip6addr((char const*)url, &http.svr, ID_SOC_DNS, dns_addr);
    }
    if (ercd != E_OK) {
        return ercd;
    }

    /* Send GET request and process response */
    ercd = ws_http_con(&http, (UB*)url);
    if (ercd != E_OK) {
        return ercd;
    }

    /* do client process */
    memset(&g_cli_ws, 0, sizeof(g_cli_ws));
    g_cli_ws.sid = http.http_sid;
    g_cli_ws.state = WS_CONNECTED;
#if WS_HTTP_RX_PATCH
    if ((http.msg != NULL) && (http.rx_len != http.rd_len)) {
        g_cli_ws.http_msg_buf = http.msg->buf + http.rd_len;
        g_cli_ws.http_msg_len = http.rx_len - http.rd_len;
    }
#endif
    return E_OK;
}

#ifdef WS_AUTOBAHN

static UB ws_cli_data[WSS_APP_BUF_SIZE];
static UB ws_cli_frg_data[WSS_APP_FRG_BUF_SIZE];

/* websocket echo application */
void ws_client_process(T_WS *ws)
{
    ER ercd;
    UH srv_evt;
    UW srv_len, frg_len;
    UH frg_state, close_reason;

    frg_len = 0;
    frg_state = 0;

    for(;;) {
        close_reason = CLIENT_TERMN_CONN;
        ercd = ws_cli_rcv(ws, ws_cli_data, &srv_len, &srv_evt);
        if (ercd != E_OK) {
            break;  /* close connection */
        }
        else if (srv_evt == RX_TMO) {
            break;
        }
        else if (srv_evt == FULL_TEXT && (frg_state == FALSE)) {
            if(ws_chk_utf8(ws_cli_data, srv_len) == FALSE) {
                close_reason = NON_UTF8;
                break;
            }
            ws_cli_snd(ws, ws_cli_data, srv_len, srv_evt);
        }
        else if (srv_evt == FULL_BIN) {
            ws_cli_snd(ws, ws_cli_data, srv_len, srv_evt);
        }
        else if (srv_evt == FULL_PING) {
            ws_cli_pong(ws, ws_cli_data, srv_len);
        }
        else if (srv_evt == FULL_PONG) {
            /* do nothing */
        }
        else if (srv_evt == CLOSE) {
            ws_cli_cls(ws, NORMAL_CLOSE);
            break;
        }
        else if((srv_evt == FIRST_TEXT) || (srv_evt == FIRST_BIN)) {
            memcpy(&ws_cli_frg_data[0], &ws_cli_data[0], srv_len);
            frg_len += srv_len;
            frg_state = (srv_evt == FIRST_TEXT) ? FULL_TEXT : FULL_BIN;
        }
        else if(srv_evt == MORE_TEXT && frg_state) {
            if((frg_len + srv_len) > WSS_APP_FRG_BUF_SIZE) {
                close_reason = TOO_BIG_RESPONSE;
                break;
            }
            memcpy(&ws_cli_frg_data[frg_len], &ws_cli_data[0], srv_len);
            frg_len += srv_len;
        }
        else if(srv_evt == LAST_TEXT && frg_state) {
            if((frg_len + srv_len) > WSS_APP_FRG_BUF_SIZE) {
                close_reason = TOO_BIG_RESPONSE;
                break;
            }
            memcpy(&ws_cli_frg_data[frg_len], &ws_cli_data[0], srv_len);
            frg_len += srv_len;
            if(frg_state == FULL_TEXT) {
                if (ws_chk_utf8(ws_cli_frg_data, frg_len) == FALSE) {
                    close_reason = NON_UTF8;
                    break;
                }
            }
            ws_cli_snd(ws, ws_cli_frg_data, frg_len, frg_state);
            frg_len = 0;
            frg_state = 0;
        }
        else {
            close_reason = PROTOCOL_ERROR;
            break;
        }
    }

    ws_cli_cls(ws, close_reason);

    return;
}

static UB url[HOST_NAME_LEN];

static void updateReports(void)
{
    ER ercd;

    sprintf((char *)url, "%s/updateReports?agent=%s", wsuri, "ws");

	/*printf("Updating reports .. \r\n");*/
    ercd = ws_cli_con((UB*)url);
    if( ercd == E_OK ) {
        /*printf("Reports updated. \r\n");*/
        /*printf("Test suite finished! \r\n");*/
    }

    ws_cli_cls(&g_cli_ws, NORMAL_CLOSE);
    http_cls(&http);
}

static ER ws_cli_test(void)
{
    ER ercd;
    int caseId;

    for (caseId=1;caseId<=517;caseId++) {
        /*printf("Test: id %d\r\n", caseId);*/
        sprintf((char *)url, "%s/runCase?case=%d&agent=%s", wsuri, caseId, "ws");
        ercd = ws_cli_con(url);
        if (ercd != E_OK) {
            http_cls(&http);
            return ercd;
        }
        ws_client_process(&g_cli_ws);
        http_cls(&http);
    }

    updateReports();
    /*printf("Test complete\r\n");*/
    return E_OK;
}

#else

static UB ws_cli_data[WSS_APP_BUF_SIZE];
#define WSS_SEND_DATA   "uNet3 Websocket demo"

ER ws_echo_cli_sample(VB *ipaddr, VB *port, UB use_ssl)
{
    T_WS *ws = &g_cli_ws;
    UH rcv_evt;
    UW snd_len,rcv_len;
    ER ercd;
    SIZE cha_len;
    UH n=0U;

    // make url.
    // wsuri[HOST_NAME_LEN] = "http://192.168.100.5:9001";
    memset(&wsuri[n], 0, sizeof(wsuri));
    if (use_ssl) {
        strcpy((VB*)&wsuri[n], "https://");
    }
    else {
        strcpy((VB*)&wsuri[n], "http://");
    }
    n += strlen((VB*)&wsuri[n]);
    
    cha_len = strlen(ipaddr);
    memcpy(&wsuri[n], ipaddr, cha_len);
    n += ((UH)cha_len);
    wsuri[n++] = ':';
    cha_len = strlen(port);
    memcpy(&wsuri[n], port, cha_len);


    /* Establish connection to websocket server */
    ercd = ws_cli_con(wsuri);
    if (ercd != E_OK) {
        http_cls(&http);
        return ercd;
    }
    ws->ssl = use_ssl;

    snd_len = DEF_WS_FRAME_MAX_PAYLOAD_SIZE; /*125 bytes*/

    strcpy((char *)&ws_cli_data[0], WSS_SEND_DATA);

    /* send message every 1 second */
    for (;;) {
        /* Send message to Server */
        ercd = ws_cli_snd(ws, &ws_cli_data[0], snd_len, FULL_TEXT);
        if (ercd != E_OK) {
            break;
        }
        /* Wait for message from Server */
        memset(&ws_cli_data[0], 0, sizeof(ws_cli_data));
        ercd = ws_cli_rcv(ws, &ws_cli_data[0], &rcv_len, &rcv_evt);
        if (ercd != E_OK) {
            if (ercd == E_TMOUT) {
                tslp_tsk(1000);  /* 1 sec delay */
                continue;
            }
            break;  /* close connection */
        }
        if (rcv_evt != FULL_TEXT) {
            break; /* expected echo message from server */
        }
        /* message received from server. verify the message */
        if (snd_len != rcv_len) {
            /*
             * The WebSocekt server may send messages manually, so make sure
             * that the strings you send and the strings you receive match.
             */
            if (memcmp(WSS_SEND_DATA, &ws_cli_data[0], strlen(WSS_SEND_DATA))) {
                break;
            }
        }

        tslp_tsk(1000);  /* 1 sec delay */
    }

    ercd = ws_cli_cls(ws, CLIENT_TERMN_CONN);

    http_cls(&http);

    return ercd;
}
#endif

ER ws_cli_sample(void)
{
#ifdef WS_AUTOBAHN
    ws_cli_test();
#else
    ws_echo_cli_sample(WS_SERVER_IP, WS_SERVER_PORT, 0);
#endif
    return E_OK;
}