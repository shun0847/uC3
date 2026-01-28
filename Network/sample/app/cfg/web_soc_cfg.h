/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    WebSocket Configuration

    Generated at 2022-02-09 15:18:00
 **************************************************************************/

#ifndef WEB_SOC_CFG_H
#define WEB_SOC_CFG_H

#ifdef __cplusplus
extern "C" {
#endif 

/* Maximum payload in a Websocket Frame 
Default: 125 bytes, Min: 125, Max: 4*1024*1024*1024   (4GB) 
*/
#define WS_FRAME_MAX_PAYLOAD_SIZE (2*1024)   /* Maximum payload size */ 


#ifdef __cplusplus
}
#endif


#endif /* WEB_SOC_CFG_H */
