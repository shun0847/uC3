/***********************************************************************
    MICRO C CUBE / COMPACT, MQTT Protocol
    MQTT Client protocol definition
    Copyright (c) 2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.08.01: Created.
************************************************************************/

#ifndef __MQTT_MSG_H__
#define __MQTT_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* MQTT Server Listening TCP Port value */
#define MQTT_SERVER_PORT            1883 /* Unencrypted */
#define MQTT_SSL_SERVER_PORT        8883 /* Encrypted */
#define MQTT_SSL_CERT_SERVER_PORT   8884 /* Encrypted with certificate */

#define MQTT_GET_PKT_RETAIN(b) (((b))&0x1)
#define MQTT_GET_PKT_QOS(b)    (((b)>>1)&0x3)
#define MQTT_GET_PKT_DUP(b)    (((b)>>3)&0x1)
#define MQTT_GET_PKT_TYPE(b)   (((b)>>4)&0xF)

#define MQTT_GET_PKT_CLEAN_SESSION(b)  (((b)>>1)&0x1)
#define MQTT_GET_PKT_WILL_FLAG(b)      (((b)>>2)&0x1)
#define MQTT_GET_PKT_WILL_QOS(b)       (((b)>>3)&0x3)
#define MQTT_GET_PKT_WILL_RETAIN(b)    (((b)>>5)&0x1)
#define MQTT_GET_PKT_WILL_PASS_FLAG(b) (((b)>>6)&0x1)
#define MQTT_GET_PKT_WILL_USER_FLAG(b) (((b)>>7)&0x1)

#define MQTT_GET_UH(b)       ( ((((UH)(b[0]))&0x00FF)<<8) | (((UH)b[1])&0x00FF) )
#define MQTT_GET_PKT_ID(b)   MQTT_GET_UH((b))
#define MQTT_GET_PKT_KEEP_ALIVE(b)  MQTT_GET_UH((b))

void mqtt_loc(T_MQTT_CLIENT *p_cli);
void mqtt_ulc(T_MQTT_CLIENT *p_cli);

ENUM_MQTT_SESSION_STATE mqtt_get_state(T_MQTT_CLIENT *p_cli);
void mqtt_set_state(T_MQTT_CLIENT *p_cli, ENUM_MQTT_SESSION_STATE state);

/* Message buffer */
ER mqtt_get_snd_buf(T_MQTT_CLIENT *p_cli, T_MQTT_MSG **pp_msg, TMO tmo);
ER mqtt_ret_snd_buf(T_MQTT_MSG *p_msg);
ER mqtt_get_rcv_buf(T_MQTT_CLIENT *p_cli, T_MQTT_MSG **pp_msg, TMO tmo);
ER mqtt_ret_rcv_buf(T_MQTT_MSG *p_msg);

/* Message construction */
ER mqtt_enc_con(T_MQTT_MSG *msg, T_MQTT_CON *con);
ER mqtt_enc_pub(T_MQTT_MSG *msg, T_MQTT_PUB *pub);
ER mqtt_enc_sub(T_MQTT_MSG *msg, T_MQTT_SUB *sub);
ER mqtt_enc_unsub(T_MQTT_MSG *msg, T_MQTT_UNSUB *unsub);
ER mqtt_enc_ping(T_MQTT_MSG *msg);
ER mqtt_enc_disconnect(T_MQTT_MSG *msg);

ER mqtt_enc_pubrel(T_MQTT_MSG *p_msg, UH pkt_id);

ER mqtt_dec_con_ack(T_MQTT_MSG *p_msg, T_MQTT_CON_ACK *p_conack);
ER mqtt_dec_sub_ack(T_MQTT_MSG *p_msg, T_MQTT_SUB_ACK *p_suback);
ER mqtt_dec_pub(T_MQTT_MSG *p_msg, T_MQTT_PUB *p_pub);

/* Message Transmission */
ER mqtt_snd(T_MQTT_MSG *msg, TMO tmo);             // add to transmit queue
void mqtt_snd_wait_wakeup(T_MQTT_MSG *p_msg);

/* Message Reception */
T_MQTT_MSG *mqtt_rcv(T_MQTT_CLIENT *p_cli);
ER mqtt_rcv_done(T_MQTT_MSG *msg);

/* Task functions */
void mqtt_snd_tsk_process(T_MQTT_CLIENT *session);
void mqtt_rcv_tsk_process(T_MQTT_CLIENT *session);

ER mqtt_ini(T_MQTT_CLIENT *session);
ER mqtt_ext(T_MQTT_CLIENT *session);

void mqtt_cls_que(T_MQTT_CLIENT *p_cli);
/*****/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __MQTT_MSG_H__ */
