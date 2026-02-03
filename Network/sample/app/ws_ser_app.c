/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    WebSocket Implementation
    Copyright (c)  2021-2024, eForce Co., Ltd. All rights reserved.

    Version Information
      2021.07.01: Created
      2023.07.25: Changes due to uNet3-HTTPd source update.
      2024.07.08: Update for HTTPS. 
      2024.10.11: Fixed the issue of not being able to connect with FireFox.
 ***************************************************************************/

/* {{UC3_INCLUDE */
#include "kernel.h"
#include "net_hdr.h"
#ifdef NET_C_OS
#include "kernel_id.h"
#include "net_id.h"
#endif

#include "net_strlib.h"
#include "base64calc.h"
#include "ssl_sha.h"
/* }}UC3_INCLUDE */
#include "http_server.h"
#include "web_soc.h"

/*Macros for calling websocket headers */

#define HTTP_VERSION "HTTP/1.1 "
#define HTTP_STATUS_LINE "101 " 
#define HTTP_RESPONSE_PHRASE "Switching Protocols\r\n"
#define WS_ACCEPT "Sec-WebSocket-Accept: " 

extern UW HttpTcpSend(T_HTTP_SERVER* http, const UB *data, UW len);
extern char parse_tok(T_HTTP_SERVER *sess, char tok, char **par);
extern char* trim_ows(char *str);
extern ER ws_srv_key(UB *key, UB *hash);

T_WS g_ws;

ER WsSendResponse(T_HTTP_SERVER *http, UB *key, UB *conn)
{
    UB hash[29];
    /* Constructing Server Response for client*/
    HttpSetContent(http, 0);
    HttpSetContent(http, HTTP_VERSION);
    HttpSetContent(http, HTTP_STATUS_LINE);
    HttpSetContent(http, HTTP_RESPONSE_PHRASE);

    HttpSetContent(http, "Upgrade: websocket\r\n");
    HttpSetContent(http, "Connection: ");
    HttpSetContent(http, (VB*)conn);
    HttpSetContent(http, "\r\n");

    /* Server constructing accept */
    /* Key and GUID to be concatenated here with SHA1 and base 64 return of it */ 
    HttpSetContent(http, WS_ACCEPT); /*YET TO BE DONE*/
    ws_srv_key(key, hash);
    hash[28] = '\0';
    HttpSetContent(http, (char*)hash);
    HttpSetContent(http, "\r\n");
    HttpSetContent(http, "\r\n");

    /* Sending the data to client*/
    HttpTcpSend(http, http->sbuf, http->txlen);

    return E_OK;
}

UB ws_data[WS_FRAME_MAX_PAYLOAD_SIZE];

/* websocket server process
    -wait for websocket frames from client
     if receive data then send it back to client
     if receive ping then send pong to client
     if receive close then send close acknowledgement to client (closes the connection)
     if receive time out
      start sending ping. If no response for three consecutive ping then close the connection.
*/
void ws_server_process(T_WS *ws)
{
    ER ercd;
    UH srv_evt;
    UW srv_len;
    UH idle_ping_count;
    
    idle_ping_count = 0;

    for(;;) {

        ercd = ws_ser_rcv(ws, ws_data, &srv_len, &srv_evt);
        if (ercd != E_OK) {
            break;  /* close connection */
        }
        else if (srv_evt == RX_TMO) {
            /* nothing received for idle time */
            /* send ping for 3 times and close connection after that */
            idle_ping_count++;
            if (idle_ping_count > 3) {
                break;  /* close connection */
            }
            else {
                ws_ser_ping(ws, NULL, 0);
            }
        }
        else if (srv_evt == FULL_TEXT) {
            idle_ping_count = 0;
            ws_ser_snd(ws, ws_data, srv_len, srv_evt);
        }
        else if (srv_evt == FULL_PING) {
            idle_ping_count = 0;            
            ws_ser_pong(ws, ws_data, srv_len);
        }
        else if (srv_evt == CLOSE) {
            ws_ser_cls(ws, NORMAL_CLOSE);
            break;
        }
        else {
            break; 
        }
    }
    
    ws_ser_cls(ws, SERVER_TERMN_CONN);
    
    return;
}

ER ws_server_callback(T_HTTP_SERVER *sess)
{
    UB c, *ws_connection, *ws_upgrade, *ws_origin, *ws_version, *ws_key;
    ER ercd;
    char *hdr = NULL;

    ws_connection = NULL;
    ws_upgrade = NULL;
    ws_origin = NULL;
    ws_version = NULL;
    ws_key = NULL;

    /* Check for all the websocket header values and send Suceess response */
    if(sess != NULL){
        c = 1;
        parse_tok(sess, '\n', &hdr);
        hdr = NULL;
        for (;;) {
            if ((c == 1)&& (hdr == NULL)) {
                if (net_strncasecmp((char const *)sess->req,"\r\n",2) == 0) {
                    sess->req += 2;
                    ercd = E_OK;
                    break;
                }
            }
            
            c = parse_tok(sess, ':', &hdr);
                if (hdr == NULL) {
                    if (!c) {   /* End of the string */
                    return E_CLS;
                    }
                continue;   /* End of Line */
            }     
            
            /*Checking header of websockets*/
            if((hdr != NULL) && (net_strncasecmp(hdr, "Connection:", 11) == 0)) {
                c = parse_tok(sess, '\n', &hdr);
                if(!c)
                    return E_CLS;
                if(hdr) {
                    if ((net_strncasecmp(hdr, "upgrade", 7) ==0) || 
                        (net_strncasecmp(hdr, "keep-alive, Upgrade", 19) ==0))  /* FireFox */
                    {   
                        *(sess->req -2) = 0;
                        ws_connection = (UB*)trim_ows(hdr);
                    }
                    hdr = NULL;             
                }
            }
            if((hdr != NULL) && (net_strncasecmp(hdr, "Upgrade:", 8) == 0)) {
                c = parse_tok(sess, '\n', &hdr);
                if (!c)
                    return E_CLS;
                if (hdr) {
                    *(sess->req - 2) = 0; 
                    ws_upgrade = (UB*)trim_ows(hdr);
                    hdr = NULL;
                }
            }
            else if((hdr != NULL) && (net_strncasecmp(hdr, "Origin:", 7) == 0 )) {
                c = parse_tok(sess, '\n', &hdr);
                if (!c)
                    return E_CLS;
                if (hdr) {
                    *(sess->req - 2) = 0;
                    ws_origin = (UB*)trim_ows(hdr);
                    hdr = NULL;
                }
            }
            else if((hdr != NULL) && (net_strncasecmp(hdr, "Sec-WebSocket-Version:", 22) == 0 )) {
                c = parse_tok(sess, '\n', &hdr);
                if (!c)
                   return E_CLS;
                if (hdr) {
                    *(sess->req - 2) = 0;
                    ws_version =  (UB*)trim_ows(hdr);
                    hdr = NULL;
                }
            }
            
            else if((hdr != NULL) && (net_strncasecmp(hdr, "Sec-WebSocket-Key:", 18) == 0)) {
                c = parse_tok(sess, '\n', &hdr);
                if (!c)
                    return E_CLS;
                if (hdr) {
                  /* ToDo Check value of key - length header value*/ 
                  *(sess->req - 2) = 0;
                  ws_key = (UB*)trim_ows(hdr);
                  hdr = NULL;            

                } 
            }
        } /*for loop ends */

        /* validate websocket headers and values */
        if (ws_connection == NULL) {
            HttpSendErrorResponse(sess, "400 Bad Request \r\n");
            return E_CLS;

        }
        if (ws_upgrade == NULL) {
            HttpSendErrorResponse(sess, "400 Bad Request \r\n");
            return E_CLS;

        }
        if (net_strncasecmp((char const*)ws_upgrade, "websocket", 9) !=0) {
            HttpSendErrorResponse(sess, "426 Upgrade required \r\n");
            return E_CLS;
        }
        if (ws_version == NULL ){
            HttpSendErrorResponse(sess, "426 Upgrade required \r\n");
            return E_CLS;
        }
        if (ws_key == NULL) {
            HttpSendErrorResponse(sess, "400 Bad Request \r\n");
            return E_CLS;    
        }
        if(ws_origin == NULL){
            /*HttpSendErrorResponse(sess, "400 Bad Request \r\n");*/
            /*return E_CLS;*/
        }        
        /* send webSocket response */
        ercd = WsSendResponse(sess, ws_key, ws_connection);
        if (ercd != E_OK) {
            return E_CLS;
        }

        /* connection established! run server process */
        net_memset(&g_ws, 0, sizeof(g_ws));
        g_ws.sid = sess->SocketID;
        g_ws.state = WS_CONNECTED;

        if (sess->flag == HTTPD_FLG_SSL) {
            g_ws.ssl = 1;
        }

        ws_server_process(&g_ws);
    }
    
    return E_CLS;  
}