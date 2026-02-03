/***********************************************************************
    MICRO C CUBE / COMPACT, MQTT Protocol
    MQTT Client API header file
    Copyright (c) 2016-2022, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.08.01: Created.
      2016.12.19: Added variable for SSL session ID to T_MQTT_CLIENT.
      2018.05.15: type of socket id are changed ID to SID.
      2021.08.25: Supports Cube Suite compiler.
      2022.12.14: Added error informatoins.
************************************************************************/

#ifndef __MQTT_H__
#define __MQTT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mqtt_util.h"
#include "net_hdr.h"

#define MQTT_MIN_PKT_SIZE   32
#define MQTT_DEF_PKT_SIZE   256

typedef enum enum_mqtt_pkt_type
{
    MQTT_PKT_TYPE_NONE    = 0,
    MQTT_PKT_TYPE_CONNECT = 1,
    MQTT_PKT_TYPE_CONNACK = 2,
    MQTT_PKT_TYPE_PUBLISH = 3,
    MQTT_PKT_TYPE_PUBACK  = 4,
    MQTT_PKT_TYPE_PUBREC  = 5,
    MQTT_PKT_TYPE_PUBREL  = 6,
    MQTT_PKT_TYPE_PUBCOMP = 7,
    MQTT_PKT_TYPE_SUBSCRIBE = 8,
    MQTT_PKT_TYPE_SUBACK    = 9,
    MQTT_PKT_TYPE_UNSUBSCRIBE = 10,
    MQTT_PKT_TYPE_UNSUBACK = 11,
    MQTT_PKT_TYPE_PINGREQ  = 12,
    MQTT_PKT_TYPE_PINGRESP = 13,
    MQTT_PKT_TYPE_DISCONNECT = 14,
}ENUM_MQTT_PKT_TYPE;

typedef enum enum_mqtt_err_cause
{
    MQTT_ERR_NONE = 0,
    MQTT_ERR_NOMEM,
    MQTT_ERR_NOT_CLOSE,
    MQTT_ERR_NOT_CONNECT,
    MQTT_ERR_SOCKET,
    MQTT_ERR_TCP_CON,
    MQTT_ERR_TCP_SND,
    MQTT_ERR_TLS_CON,
    MQTT_ERR_TLS_SND,
    MQTT_ERR_INV_CID,
    MQTT_ERR_INV_TOPIC,
    MQTT_ERR_INV_WILL,
    MQTT_ERR_INV_QOS,
    MQTT_ERR_INV_USRPAS,
    MQTT_ERR_INV_MSG_SIZE,
    MQTT_ERR_INV_RESPONSE,
    MQTT_ERR_CMD_TIMOUT,
    MQTT_ERR_CMD_CANCEL,
    MQTT_ERR_CMD_UNRES,
    MQTT_ERR_CMD_ERRRES,
    MQTT_ERR_INTERNAL,
}ENUM_MQTT_ERR_CAUSE;

typedef struct t_mqtt_str
{
    UH len;   /* Byte1: String length MSB, Byte2: String length LSB */
    UB *p_val;
}T_MQTT_STR;

typedef struct t_mqtt_con
{
    T_MQTT_STR  client_id;  // trunc 23 bytes,
    T_MQTT_STR  will_topic;
    T_MQTT_STR  will_message;
    T_MQTT_STR  user;
    T_MQTT_STR  pass;
    UB          will_qos;
    UB          will_retain;
    UH          keep_alive;
    UB          clean_session;
    T_NODE      broker;
    BOOL        use_ssl;
    /* parameters return by API */
    ENUM_MQTT_ERR_CAUSE cause;
    UB          sp;
    UB          return_code;
}T_MQTT_CON;

typedef struct t_mqtt_con_ack
{
    UB          sp;
    UB          return_code;
}T_MQTT_CON_ACK;

typedef struct t_mqtt_pub
{
    T_MQTT_STR          topic;
    ENUM_MQTT_PKT_TYPE  cmd;
    UH          pkt_id;
    UB          qos;
    UB          retain;
    UB          dup;
    UH          payload_len;
    UB          *p_payload;
    /* parameter return by API */
    ENUM_MQTT_ERR_CAUSE cause;
}T_MQTT_PUB;

typedef struct t_mqtt_pub_ack
{
    UH          pkt_id;
}T_MQTT_PUB_ACK;

typedef T_MQTT_PUB_ACK T_MQTT_PUB_REL;
typedef T_MQTT_PUB_ACK T_MQTT_PUB_REC;
typedef T_MQTT_PUB_ACK T_MQTT_PUB_COMP;

typedef struct t_mqtt_sub_topic
{
    T_MQTT_STR  topic;
    UB          qos;
    UB          return_code; /* parameter return by API */
}T_MQTT_SUB_TOPIC;

typedef struct t_mqtt_sub
{
    UH          pkt_id;
    UB          topic_count;
    T_MQTT_SUB_TOPIC *p_topic_list;
    /* parameter return by API */
    ENUM_MQTT_ERR_CAUSE cause;
}T_MQTT_SUB;

typedef struct t_mqtt_sub_ack
{
    UH          pkt_id;
    UB          topic_count;
    UB          *p_return_code;
}T_MQTT_SUB_ACK;

typedef struct t_mqtt_unsub
{
    UH          pkt_id;
    UB          topic_count;
    T_MQTT_STR  *p_topic;
    /* parameter return by API */
    ENUM_MQTT_ERR_CAUSE cause;
}T_MQTT_UNSUB;

typedef T_MQTT_PUB_ACK T_MQTT_UNSUB_ACK;

typedef enum enum_mqtt_session_stat
{
    MQTT_SESS_NONE = 0,
    MQTT_SESS_CLOSED,
    MQTT_SESS_CONNECTING,
    MQTT_SESS_CONNECTED,
    MQTT_SESS_CLOSING,
}ENUM_MQTT_SESSION_STATE;

typedef struct t_mqtt_client
{
    /* Application parameters */
    SID soc_id;            /* TCP socket ID */
    ID ssn_id;            /* SSL session ID */
    UH max_msg_size;      /* MQTT maximum message size */
    ID loc_semid;         /* Semaphore ID */
    ID evt_flgid;         /* Event flag ID */
    ID tx_mpfid;          /* Tx command buffer */
    ID rx_mpfid;          /* Rx command buffer */
    ID tx_tskid;          /* Tx task */
    ID rx_tskid;          /* Rx task */
    ID rx_app_tskid;      /* Rx app task */
    BOOL (*cbk)(struct t_mqtt_client *, struct t_mqtt_pub *);

    /* Internal parameters */
    T_MQTT_LST tx_que;        /* Messages wait for transmission */
    T_MQTT_LST tx_ack_wai_que; /* Transmitted messages wait for ACK reception */
    T_MQTT_LST rx_que;         /* Received message wait to deliver to application */
    struct t_mqtt_msg *p_cbk_msg; /* Message in cbk function */
    UB use_ssl;           /* use secure socket or not */
    ENUM_MQTT_SESSION_STATE state; /* Session State */
}T_MQTT_CLIENT;

#define MQTT_EV_CON     0x0001
#define MQTT_EV_TX_RDY  0x0002
#define MQTT_EV_RX_RDY  0x0004

typedef enum enum_mqtt_msg_state
{
    MQTT_MSG_STATE_NONE = 0,
    MQTT_MSG_STATE_IDLE,
    MQTT_MSG_STATE_TX_PENDING,   /* tx_que, wait for transmission */
    MQTT_MSG_STATE_TRANSMITTING, /* tx_que, transmitting */
    MQTT_MSG_STATE_TX_WAIT_ACK,  /* tx_ack_wai_que, wait for ACK reception */
    //MQTT_MSG_STATE_RX_START,
    MQTT_MSG_STATE_RX_CMD,
    //MQTT_MSG_STATE_RX_ACK,
    MQTT_MSG_STATE_ABORT,
    MQTT_MSG_STATE_DONE,
    MQTT_MSG_STATE_ERROR,
}ENUM_MQTT_MSG_STATE;

typedef struct t_mqtt_msg
{
    T_MQTT_LST_ELE  *p_nxt;
    T_MQTT_LST_ELE  *p_prv;
    ID          mpfid;
    BOOL        wai_flg;
    ID          wai_tskid;
    T_MQTT_CLIENT *p_cli;
    struct t_mqtt_msg *p_pkt2;
    ENUM_MQTT_MSG_STATE state;
    ENUM_MQTT_ERR_CAUSE cause;
    ER ercd;                    //E_OK - success, ! = E_OK error
    UH pkt_id;                  // message identifier
    UH pkt_len;                 // MQTT Control Packet Len
    UB pkt[4];                  // MQTT Control Packet
}T_MQTT_MSG;


ER mqtt_cli_cre(T_MQTT_CLIENT *p_cli);
ER mqtt_cli_del(T_MQTT_CLIENT *p_cli);
ER mqtt_con(T_MQTT_CLIENT *p_cli, T_MQTT_CON *p_con, TMO tmo);
ER mqtt_cls(T_MQTT_CLIENT *p_cli, TMO tmo);
ER mqtt_pub(T_MQTT_CLIENT *p_cli, T_MQTT_PUB *p_pub, TMO tmo);
ER mqtt_sub(T_MQTT_CLIENT *p_cli, T_MQTT_SUB *p_sub, TMO tmo);
ER mqtt_unsub(T_MQTT_CLIENT *p_cli, T_MQTT_UNSUB *p_unsub, TMO tmo);
ER mqtt_ping(T_MQTT_CLIENT *p_cli, TMO tmo);
//BOOL mqtt_rcv_callback(T_MQTT_CLIENT *p_cli, T_MQTT_PUB *p_pub);

/*****/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __MQTT_H__ */
