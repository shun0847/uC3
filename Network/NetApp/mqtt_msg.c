/***********************************************************************
    MICRO C CUBE / COMPACT, MQTT Protocol
    MQTT Client protocol
    Copyright (c) 2016-2025, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.08.01: Created.
      2016.12.27: Improved of looping problem in mqtt_rx_process().
      2022.10.25: Bug fixing
                    - Leakage of RX buffer release in mqtt_cls_que().
      2022.12.14: Added error informatoins.
      2025.01.28: Fixed a problem where mqtt_cls() unintentionally RST.
************************************************************************/

#include <string.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_strlib.h"
#include "mqtt_util.h"
#include "mqtt.h"
#include "mqtt_msg.h"

#ifndef NULL
#ifdef __cplusplus
#define NULL    (0)
#else
#define NULL    ((void *)0)
#endif
#endif

#define MQTT_CTL_PKT_SIZE  p_msg->p_cli->max_msg_size

extern ER mqtt_tcp_send(T_MQTT_CLIENT *p_cli, UB *data, UH len);
extern ER mqtt_tcp_recv(T_MQTT_CLIENT *p_cli, UB *data, UH len);
extern ER mqtt_tcp_cls(T_MQTT_CLIENT *p_cli, TMO tmo);

/* lock all entry points */
void mqtt_loc(T_MQTT_CLIENT *p_cli)
{
    if (p_cli) {
        wai_sem(p_cli->loc_semid);
    }
}

void mqtt_ulc(T_MQTT_CLIENT *p_cli)
{
    if (p_cli) {
        sig_sem(p_cli->loc_semid);
    }
}

void mqtt_ini_buf(T_MQTT_CLIENT *p_cli, ID mpfid, T_MQTT_MSG *p_msg)
{
    memset(p_msg, 0, sizeof(T_MQTT_MSG));
    p_msg->mpfid = mpfid;
    p_msg->state = MQTT_MSG_STATE_IDLE;
    p_msg->p_cli = p_cli;
    p_msg->cause  = MQTT_ERR_NONE;
}

ENUM_MQTT_SESSION_STATE mqtt_get_state(T_MQTT_CLIENT *p_cli)
{
    ENUM_MQTT_SESSION_STATE state;

    state =  p_cli->state;

    return state;
}

void mqtt_set_state(T_MQTT_CLIENT *p_cli, ENUM_MQTT_SESSION_STATE state)
{
    p_cli->state = state;
}

ER mqtt_get_snd_buf(T_MQTT_CLIENT *p_cli, T_MQTT_MSG **pp_msg, TMO tmo)
{
    ER ercd;

    ercd = tget_mpf(p_cli->tx_mpfid, (VP)pp_msg, tmo);
    if (ercd == E_OK) {
        mqtt_ini_buf(p_cli, p_cli->tx_mpfid, *pp_msg);
    }

    return ercd;
}

ER mqtt_ret_snd_buf(T_MQTT_MSG *p_msg)
{
    T_MQTT_MSG *p_msg2;

    if (p_msg == NULL) {
        return E_PAR;
    }

    p_msg2 = (T_MQTT_MSG *)p_msg->p_pkt2;
    if (p_msg2) {
        rel_mpf(p_msg2->mpfid, p_msg2);
    }

    rel_mpf(p_msg->mpfid, p_msg);
    return E_OK;
}

ER mqtt_get_rcv_buf(T_MQTT_CLIENT *p_cli, T_MQTT_MSG **pp_msg, TMO tmo)
{
    ER ercd;

    ercd = tget_mpf(p_cli->rx_mpfid, (VP)pp_msg, tmo);
    if (ercd == E_OK) {
        mqtt_ini_buf(p_cli, p_cli->rx_mpfid, *pp_msg);
    }

    return ercd;
}

ER mqtt_ret_rcv_buf(T_MQTT_MSG *p_msg)
{
    return mqtt_ret_snd_buf(p_msg);
}

UH mqtt_gen_pkt_id(void)
{
    static UH gMQTT_pkt_id;
    
    gMQTT_pkt_id++;
    if (gMQTT_pkt_id == 0) {
        gMQTT_pkt_id++;
    }

    return gMQTT_pkt_id;
}

static void mqtt_set_ctl_flag(UB *ctl, UB type, UB dup, UB qos, UB retain)
{
    *ctl = 0;
    *ctl |= (retain & 0x01);
    *ctl |= (qos & 0x03) << 1;
    *ctl |= (dup & 0x01) << 3;
    *ctl |= (type & 0x0F) << 4;
}

UH mqtt_enc_utf8(UB *p, UB *dat, UH len)
{
    p[0] = (len >> 8) & 0xFF;
    p[1] = len & 0xFF;
    memcpy(&p[2], dat, len);

    return len + 2;
}

static UB mqtt_encode_rlen(UB *p, UW len)
{
    UB *byte;
    
    byte = p;
    
    /* Refer 2.2.3 Remaining Length */
    do {
        *byte = len % 128;
        len = len / 128;
        if (len > 0) {
            *byte |= 128; /* set MSB */
        }
        byte++;
    } while (len > 0);

    return (byte - p);  /* no. of bytes used */
}

static UB mqtt_get_len_bytes(UB *p)
{
    UB n, i;

    n = 0;
    for (i=0;i<4;i++) {
        n++;
        if ((p[i] & 0x80) != 0x80) {
            break;
        }
        if (i == 3) {
            n = 0;  /* invalid length */
            break;
        }
    }

    return n;
}

UW mqtt_dec_rlen(UB *p, UB n)
{
    UW val;
    UH mul;
    int i;

    val = 0;
    mul = 1;

    for (i=0;i<n;i++) {
        val += (p[i] & 127) * mul;
        mul *= 128;
    }

    return val;
}

static void mqtt_bld_pub_res(T_MQTT_MSG *p_msg, UB pkt_type)
{
    UB *p;

    p = p_msg->pkt;

    /* overwrite command and length */
    p[0] = (pkt_type << 4) & 0xF0;
    p[1] = 2;
    p[2] = (p_msg->pkt_id >> 8) & 0xFF; /* MSB */
    p[3] = p_msg->pkt_id & 0xFF;        /* LSB */

    p_msg->pkt_len = 4;
}

void mqtt_enc_ack(T_MQTT_MSG *p_msg, UB cmd)
{
    UB *p;

    p = p_msg->pkt;

    p[0] = (cmd << 4) & 0xF0;
    p[1] = 2;
    p[2] = (p_msg->pkt_id >> 8) & 0xFF;
    p[3] = (p_msg->pkt_id) & 0xFF;

    p_msg->pkt_len = 4;
}

ER mqtt_enc_con(T_MQTT_MSG *p_msg, T_MQTT_CON *p_con)
{
    UB *p;
    UH len;

    if (p_msg == NULL || p_con == NULL) {
        return E_PAR;
    }    

    if (p_msg->state != MQTT_MSG_STATE_IDLE) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }    
    
    if (!(mqtt_chk_client_id(p_con->client_id.p_val, p_con->client_id.len))) {
        p_msg->cause = MQTT_ERR_INV_CID;
        return E_PAR;
    }

    if (p_con->will_topic.len != 0) {

        if (!(mqtt_chk_pub_topic(p_con->will_topic.p_val, p_con->will_topic.len))) {
            p_msg->cause = MQTT_ERR_INV_TOPIC;
            return E_PAR;
        }
        
        if (p_con->will_message.len == 0 || p_con->will_message.p_val == NULL) {
            p_msg->cause = MQTT_ERR_INV_WILL;
            return E_PAR;
        }
        
        if (p_con->will_qos > 2) {
            p_msg->cause = MQTT_ERR_INV_QOS;
            return E_PAR;
        }   
    }
    
    if (p_con->user.len != 0) {

        if (!(mqtt_chk_user_name(p_con->user.p_val, p_con->user.len))) {
            p_msg->cause = MQTT_ERR_INV_USRPAS;
            return E_PAR;
        }
    }
    
    if (p_con->pass.len != 0) {
        if (p_con->pass.p_val == NULL) {
            p_msg->cause = MQTT_ERR_INV_USRPAS;
            return E_PAR;
        }
        /* if password exists the user name should exists */
        if (p_con->user.p_val == NULL) {
            p_msg->cause = MQTT_ERR_INV_USRPAS;
            return E_PAR;
        }
    }
    
    p = p_msg->pkt;
    
    /* set flags */
    mqtt_set_ctl_flag(&p[0], MQTT_PKT_TYPE_CONNECT, 0, 0, 0);
    p++;
    
    /* calculate and set remaining length
       6 bytes protocol name
       1 byte protocol level
       1 byte connect flag
       2 bytes keep alive
       n client identifier
       n Will Topic
       n Will Message
       n User
       n Password
    */
    len = 10 + p_con->client_id.len + 2;
    if (p_con->will_topic.len)
        len += p_con->will_topic.len + 2;
    if (p_con->will_message.len)
        len += p_con->will_message.len + 2;
    if (p_con->user.len)
        len += p_con->user.len + 2;
    if (p_con->pass.len)
        len += p_con->pass.len + 2;
    
    p += mqtt_encode_rlen(p, len);

    p_msg->pkt_len = len + (p - p_msg->pkt);
    if (p_msg->pkt_len > MQTT_CTL_PKT_SIZE) {
        p_msg->cause = MQTT_ERR_INV_MSG_SIZE;
        return E_BOVR;
    }

    /* Set variable header */
    
    p += mqtt_enc_utf8(p, "MQTT", 4);

    *p = 4;  /* protocol level */
    p++;

    /* Flag */
    p[0] = 0;
    p[0] |= (p_con->clean_session != 0) << 1;  /* Bit 1 - Clean session */
    if (p_con->will_topic.len != 0) {
        p[0] |= 1 << 2; /* Bit 2  - Will flag */
        p[0] |= (p_con->will_qos & 0x3) << 3;   /* Bit 3,4  - Will  QoS */
        p[0] |= (p_con->will_retain != 0) << 5; /* Bit 5  - Will retain */
    }
    if (p_con->pass.len != 0) {
        p[0] |= 1 << 6; /* Bit 6  - Password */
    }
    if (p_con->user.len != 0) {
        p[0] |= 1 << 7; /* Bit 7  - User name */
    }
    p++;

    p[0] = (p_con->keep_alive >> 8) & 0xFF; /* MSB */
    p[1] = p_con->keep_alive & 0xFF;        /* LSB */
    
    p += 2;
    
    /* set payload */
    p += mqtt_enc_utf8(p, p_con->client_id.p_val, p_con->client_id.len);
    
    if (p_con->will_topic.len) {
        p += mqtt_enc_utf8(p, p_con->will_topic.p_val, p_con->will_topic.len);
        p += mqtt_enc_utf8(p, p_con->will_message.p_val, p_con->will_message.len);
    }
    
    if (p_con->user.len != 0) {
        p += mqtt_enc_utf8(p, p_con->user.p_val, p_con->user.len);
    }

    if (p_con->pass.len != 0) {
        p += mqtt_enc_utf8(p, p_con->pass.p_val, p_con->pass.len);
    }   

    p_msg->pkt_len = p - p_msg->pkt;
    p_msg->pkt_id = mqtt_gen_pkt_id();

    return E_OK;
}

ER mqtt_enc_pub(T_MQTT_MSG *p_msg, T_MQTT_PUB *p_pub)
{
    UB *p;
    UH len;

    /* validate parameters */

    if (p_msg == NULL || p_pub == NULL) {
        return E_PAR;
    }    

    if (p_msg->state != MQTT_MSG_STATE_IDLE) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }    
    
    if (!(mqtt_chk_pub_topic(p_pub->topic.p_val, p_pub->topic.len))) {
        p_msg->cause = MQTT_ERR_INV_TOPIC;
        return E_PAR;
    }
    
    if (p_pub->qos > 2) {
        p_msg->cause = MQTT_ERR_INV_QOS;
        return E_PAR;
    }   
    
    if (p_pub->payload_len != 0 && p_pub->p_payload == NULL) {
        p_msg->cause = MQTT_ERR_INV_MSG_SIZE;
        return E_PAR;
    }
    
    p = p_msg->pkt;
    
    /* set flags */
    mqtt_set_ctl_flag(&p[0], MQTT_PKT_TYPE_PUBLISH, p_pub->dup, p_pub->qos, p_pub->retain); // type, dup, qos, retain
    p++;
    
    /* calculate and set remaining length
       n Topic
       n [Packet Identifier]
       n [Payload]
    */
    len = p_pub->topic.len + 2;
    if (p_pub->qos > 0) {
        len += 2; /* packet identifier */
    }
    len += p_pub->payload_len;

    p += mqtt_encode_rlen(p, len);

    p_msg->pkt_len = len + (p - p_msg->pkt);
    if (p_msg->pkt_len > MQTT_CTL_PKT_SIZE) {
        p_msg->cause = MQTT_ERR_INV_MSG_SIZE;
        return E_BOVR;
    }

    /* Set variable header */
    
    p += mqtt_enc_utf8(p, p_pub->topic.p_val, p_pub->topic.len);
    
    if (p_pub->pkt_id != 0)
        p_msg->pkt_id = p_pub->pkt_id;
    else
        p_msg->pkt_id = mqtt_gen_pkt_id();
    if (p_pub->qos > 0) {
        p[0] = (p_msg->pkt_id >> 8) & 0xFF; /* MSB */
        p[1] = p_msg->pkt_id & 0xFF;        /* LSB */
        p += 2;
    }
    
    /* set payload */
    memcpy(p, p_pub->p_payload, p_pub->payload_len);
    p += p_pub->payload_len;
    
    p_msg->pkt_len = p - p_msg->pkt;
    //p_msg->pkt_id = mqtt_gen_pkt_id();

    return E_OK;
}

ER mqtt_enc_pubrel(T_MQTT_MSG *p_msg, UH pkt_id)
{
    UB *p;

    /* validate parameters */

    if (p_msg == NULL) {
        return E_PAR;
    }    

    if (pkt_id == 0) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_PAR;
    }

    if (p_msg->state != MQTT_MSG_STATE_DONE && p_msg->state != MQTT_MSG_STATE_ERROR) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }
    p_msg->state = MQTT_MSG_STATE_IDLE;
    //if (p_msg->state != MQTT_MSG_STATE_IDLE) {
      //  return E_OBJ;
    //}
    
    p = p_msg->pkt;
    
    /* set flags */
    mqtt_set_ctl_flag(&p[0], MQTT_PKT_TYPE_PUBREL, 0, 1, 0);    /* Bit 1 should be set */
    p[1] = 2;   /* set remaining len */

    /* set variable header */
    p[2] = (pkt_id >> 8) & 0xFF; /* MSB */
    p[3] = pkt_id & 0xFF;        /* LSB */
    
    p_msg->pkt_len = 4;
    p_msg->pkt_id = pkt_id;

    return E_OK;
}

ER mqtt_enc_sub(T_MQTT_MSG *p_msg, T_MQTT_SUB *p_sub)
{
    UB *p, cnt;
    UH len;
    T_MQTT_SUB_TOPIC *topic;

    /* validate parameters */

    if (p_msg == NULL || p_sub == NULL) {
        return E_PAR;
    }    

    if (p_msg->state != MQTT_MSG_STATE_IDLE) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }    
    
    if (p_sub->topic_count == 0) {
        p_msg->cause = MQTT_ERR_INV_TOPIC;
        return E_PAR;
    }
    if (p_sub->p_topic_list == NULL) {
        p_msg->cause = MQTT_ERR_INV_TOPIC;
        return E_PAR;
    }    
    
    p = p_msg->pkt;
    
    /* set flags */
    mqtt_set_ctl_flag(&p[0], MQTT_PKT_TYPE_SUBSCRIBE, 0, 1, 0); /* Bit 1 should be set */
    p++;
    
    /* calculate and set remaining length
       n Packet Identifier
       n Topic + QoS
    */
    len = 2; 
    cnt = p_sub->topic_count;
    topic = p_sub->p_topic_list;
    while (cnt) {
        if (!(mqtt_chk_sub_topic(topic->topic.p_val, topic->topic.len))) {
            p_msg->cause = MQTT_ERR_INV_TOPIC;
            return E_PAR;
        }
        if (topic->qos > 2) {
            p_msg->cause = MQTT_ERR_INV_QOS;
            return E_PAR;
        }

        len += topic->topic.len + 2;
        len += 1; // qos

        if (len >= MQTT_CTL_PKT_SIZE) {/* >= included first byte */
            p_msg->cause = MQTT_ERR_INV_MSG_SIZE;
            return E_BOVR;
        }
        cnt--;
        topic++;
    }
    
    p += mqtt_encode_rlen(p, len);

    p_msg->pkt_len = len + (p - p_msg->pkt);
    if (p_msg->pkt_len > MQTT_CTL_PKT_SIZE) {
        p_msg->cause = MQTT_ERR_INV_MSG_SIZE;
        return E_BOVR;
    }

    /* Set variable header */
    p_msg->pkt_id = mqtt_gen_pkt_id();

    p[0] = (p_msg->pkt_id >> 8) & 0xFF; /* MSB */
    p[1] = p_msg->pkt_id & 0xFF;        /* LSB */
    p += 2;
    
    /* set payload */
    cnt = p_sub->topic_count;
    topic = p_sub->p_topic_list;
    while (cnt) {
        p += mqtt_enc_utf8(p, topic->topic.p_val, topic->topic.len);
        *p = topic->qos;
        p++;
        cnt--;
        topic++;
    }

    p_msg->pkt_len = p - p_msg->pkt;

    return E_OK;
}


ER mqtt_enc_unsub(T_MQTT_MSG *p_msg, T_MQTT_UNSUB *p_unsub)
{
    UB *p, cnt;
    UH len;
    T_MQTT_STR *topic;

    /* validate parameters */

    if (p_msg == NULL || p_unsub == NULL) {
        return E_PAR;
    }    

    if (p_msg->state != MQTT_MSG_STATE_IDLE) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }    
    
    if (p_unsub->topic_count == 0) {
        p_msg->cause = MQTT_ERR_INV_TOPIC;
        return E_PAR;
    }
    if (p_unsub->p_topic == NULL) {
        p_msg->cause = MQTT_ERR_INV_TOPIC;
        return E_PAR;
    }    
    
    p = p_msg->pkt;
    
    /* set flags */
    mqtt_set_ctl_flag(&p[0], MQTT_PKT_TYPE_UNSUBSCRIBE, 0, 1, 0); /* Bit 1 should be set */
    p++;
    
    /* calculate and set remaining length
       n Packet Identifier
       n Topic + QoS
    */
    len = 2; 
    cnt = p_unsub->topic_count;
    topic = p_unsub->p_topic;
    while (cnt) {
        if (!(mqtt_chk_sub_topic(topic->p_val, topic->len))) {
            p_msg->cause = MQTT_ERR_INV_TOPIC;
            return E_PAR;
        }

        len += topic->len + 2;

        if (len >= MQTT_CTL_PKT_SIZE) {/* >= included first byte */
            p_msg->cause = MQTT_ERR_INV_MSG_SIZE;
            return E_BOVR;
        }
        cnt--;
        topic++;
    }
    
    p += mqtt_encode_rlen(p, len);

    p_msg->pkt_len = len + (p - p_msg->pkt);
    if (p_msg->pkt_len > MQTT_CTL_PKT_SIZE) {
        p_msg->cause = MQTT_ERR_INV_MSG_SIZE;
        return E_BOVR;
    }

    /* Set variable header */
    p_msg->pkt_id = mqtt_gen_pkt_id();

    p[0] = (p_msg->pkt_id >> 8) & 0xFF; /* MSB */
    p[1] = p_msg->pkt_id & 0xFF;        /* LSB */
    p += 2;
    
    /* set payload */
    cnt = p_unsub->topic_count;
    topic = p_unsub->p_topic;
    while (cnt) {
        p += mqtt_enc_utf8(p, topic->p_val, topic->len);
        cnt--;
        topic++;
    }

    p_msg->pkt_len = p - p_msg->pkt;

    return E_OK;
}

ER mqtt_enc_ping(T_MQTT_MSG *p_msg)
{
    UB *p;

    /* validate parameters */

    if (p_msg == NULL) {
        return E_PAR;
    }    

    if (p_msg->state != MQTT_MSG_STATE_IDLE) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }    
    
    p = p_msg->pkt;
    
    /* set flags */
    mqtt_set_ctl_flag(&p[0], MQTT_PKT_TYPE_PINGREQ, 0, 0, 0);
    p++;
    *p = 0;
    p++;

    p_msg->pkt_len = 2;
    p_msg->pkt_id = mqtt_gen_pkt_id();

    return E_OK;
}

ER mqtt_enc_disconnect(T_MQTT_MSG *p_msg)
{
    UB *p;

    /* validate parameters */

    if (p_msg == NULL) {
        return E_PAR;
    }    

    if (p_msg->state != MQTT_MSG_STATE_IDLE) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }    
    
    p = p_msg->pkt;
    
    /* set flags */
    mqtt_set_ctl_flag(&p[0], MQTT_PKT_TYPE_DISCONNECT, 0, 0, 0);
    p++;
    *p = 0;
    p++;

    p_msg->pkt_len = 2;
    p_msg->pkt_id = mqtt_gen_pkt_id();

    return E_OK;
}

ER mqtt_dec_pub(T_MQTT_MSG *p_msg, T_MQTT_PUB *p_pub)
{
    UB *p,pkt_type,n;
    UW len, varhdr_len;

    if (p_msg == NULL || p_pub == NULL) {
        return E_PAR;
    }    

    if (p_msg->state != MQTT_MSG_STATE_RX_CMD) {
        return E_OBJ;
    }

    p = p_msg->pkt;

    pkt_type = MQTT_GET_PKT_TYPE(p[0]);
    if (pkt_type != MQTT_PKT_TYPE_PUBLISH) {
        return E_OBJ;
    }

    n = mqtt_get_len_bytes(&p[1]);
    if (n == 0) {
        return E_OBJ;
    }

    len = mqtt_dec_rlen(&p[1], n);
    if (len >= MQTT_CTL_PKT_SIZE) {/* >= included first byte */
        return E_BOVR;
    }

    /* 1st byte */
    p_pub->retain = MQTT_GET_PKT_RETAIN(p[0]);
    p_pub->qos    = MQTT_GET_PKT_QOS(p[0]);
    p_pub->dup    = MQTT_GET_PKT_DUP(p[0]);

    /* var hdr */
    p = &p[1+n];
    p_pub->topic.len   = MQTT_GET_UH(p);
    p_pub->topic.p_val = &p[2];
    varhdr_len = p_pub->topic.len + 2;
    p += varhdr_len;

    p_pub->pkt_id  = 0;
    if (p_pub->qos > 0) {
        p_pub->pkt_id  = MQTT_GET_PKT_ID(p);
        varhdr_len += 2;
        p += 2;
    }

    /* payload */
    p_pub->p_payload    = p;
    p_pub->payload_len  = len - varhdr_len;
        
    return E_OK;
}

ER mqtt_dec_con_ack(T_MQTT_MSG *p_msg, T_MQTT_CON_ACK *p_conack)
{
    UB *p,pkt_type;

    if (p_msg == NULL || p_conack == NULL) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_PAR;
    }    

    if (p_msg->state != MQTT_MSG_STATE_DONE && p_msg->state != MQTT_MSG_STATE_ERROR) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }

    p = p_msg->pkt;

    pkt_type = MQTT_GET_PKT_TYPE(p[0]);
    if (pkt_type != MQTT_PKT_TYPE_CONNECT) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }
    
    if (p_msg->p_pkt2 == NULL) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }    

    p = p_msg->p_pkt2->pkt;

    pkt_type = MQTT_GET_PKT_TYPE(p[0]);
    if (pkt_type != MQTT_PKT_TYPE_CONNACK) {
        p_msg->cause = MQTT_ERR_INV_RESPONSE;
        return E_OBJ;
    }    
    if (p[1] != 2) {
        p_msg->cause = MQTT_ERR_INV_RESPONSE;
        return E_OBJ;
    }
    
    p_conack->sp = p[2] & 1;
    p_conack->return_code = p[3];

    return E_OK;
}

ER mqtt_dec_sub_ack(T_MQTT_MSG *p_msg, T_MQTT_SUB_ACK *p_suback)
{
    UB *p,pkt_type,n;
    UW len;
    
    if (p_msg == NULL || p_suback == NULL) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_PAR;
    }    

    if (p_msg->state != MQTT_MSG_STATE_DONE && p_msg->state != MQTT_MSG_STATE_ERROR) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }

    p = p_msg->pkt;

    pkt_type = MQTT_GET_PKT_TYPE(p[0]);
    if (pkt_type != MQTT_PKT_TYPE_SUBSCRIBE) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }
    
    if (p_msg->p_pkt2 == NULL) {
        p_msg->cause = MQTT_ERR_INTERNAL;
        return E_OBJ;
    }    

    p = p_msg->p_pkt2->pkt;

    pkt_type = MQTT_GET_PKT_TYPE(p[0]);

    if (pkt_type != MQTT_PKT_TYPE_SUBACK) {
        p_msg->cause = MQTT_ERR_INV_RESPONSE;
        return E_OBJ;
    }    

    n = mqtt_get_len_bytes(&p[1]);
    if (n == 0) {
        p_msg->cause = MQTT_ERR_INV_RESPONSE;
        return E_OBJ;
    }

    len = mqtt_dec_rlen(&p[1], n);
    if (len >= MQTT_CTL_PKT_SIZE) {/* >= included first byte */
        p_msg->cause = MQTT_ERR_INV_MSG_SIZE;
        return E_BOVR;
    }
    
    /* payload */
    p = p + 1 + n + 2; /* payload = 1st byte + rem len bytes + var hdr */
    len -= 2;         /* payload len = rem len - var hdr len */

    p_suback->topic_count   = len;
    p_suback->p_return_code = p;

    return E_OK;
}

static T_MQTT_MSG* mqtt_get_que(T_MQTT_CLIENT *p_cli, T_MQTT_LST *que)
{
    T_MQTT_MSG *p_msg;

    p_msg = (T_MQTT_MSG *)ref_mqtt_lst(que, LST_TOP);
    if (p_msg) {
        rmv_mqtt_lst(que, (T_MQTT_LST_ELE *)p_msg);
    }

    return p_msg;
}

static void mqtt_add_que(T_MQTT_CLIENT *p_cli, T_MQTT_LST *que, T_MQTT_MSG *p_msg)
{
    add_mqtt_lst(que, (T_MQTT_LST_ELE *)p_msg, LST_BOTTOM);
}

static T_MQTT_MSG* mqtt_fnd_wait_que_msg(T_MQTT_CLIENT *p_cli, UB cmd, BOOL id, UH pkt_id)
{
    T_MQTT_MSG *p_msg;
    UB pkt_type;

    /* search the queued messages for matching message */
    p_msg = (T_MQTT_MSG *)ref_mqtt_lst(&p_cli->tx_ack_wai_que, LST_TOP);  // return top in the list
    while (p_msg != NULL) {
        pkt_type = MQTT_GET_PKT_TYPE(p_msg->pkt[0]);
        if (pkt_type == cmd) {
            if ((id == FALSE) || (p_msg->pkt_id == pkt_id)) {
                break;
            }
        }
        p_msg = (T_MQTT_MSG *)ref_mqtt_lst_nxt((T_MQTT_LST_ELE *)p_msg);
    }

    /* match found, remove it from queue */
    if (p_msg) {
        rmv_mqtt_lst(&p_cli->tx_ack_wai_que, (T_MQTT_LST_ELE *)p_msg);
    }

    return p_msg;
}

static void mqtt_que_pub_msg(T_MQTT_CLIENT *p_cli, T_MQTT_MSG *p_msg)
{
    mqtt_add_que(p_cli, &p_cli->rx_que, p_msg);
    set_flg(p_cli->evt_flgid, MQTT_EV_RX_RDY);
}

static void mqtt_que_snd_msg(T_MQTT_CLIENT *p_cli, T_MQTT_MSG *p_msg)
{
    mqtt_add_que(p_cli, &p_cli->tx_que, p_msg);
    set_flg(p_cli->evt_flgid, MQTT_EV_TX_RDY);
}

static void mqtt_snd_msg_done(T_MQTT_MSG *p_msg, ER ercd)
{
  if (ercd != E_OK) {
      p_msg->state = MQTT_MSG_STATE_ERROR;
      p_msg->ercd  = ercd;
  }
  else {
      p_msg->state = MQTT_MSG_STATE_DONE;
      p_msg->ercd  = E_OK;
  }

  mqtt_snd_wait_wakeup(p_msg);
}

static ER mqtt_rcv_pkt(T_MQTT_CLIENT *s, T_MQTT_MSG *p_msg, UB **pp_varhdr, TMO tmo)
{
    UW len;
    UB *p, n;
    ER ercd;

    /* wait until recv minimal header */
    p = &p_msg->pkt[0];
    ercd = mqtt_tcp_recv(s, p, 2);
    if (ercd != E_OK) {
        return ercd;
    }

    /* remaining length value greater than 127 bytes ? */
    len = p[1];
    if (p[1] & 0x80) {
        /* read three more bytes to calculate the actual length */
        ercd = mqtt_tcp_recv(s, &p[2], 3);
        if (ercd != E_OK) {
            return ercd;
        }

        n = mqtt_get_len_bytes(&p[1]);
        if (n == 0) {
            /* invalid length */
            return E_OBJ;
        }

        *pp_varhdr = p + 1 + n;

        len = mqtt_dec_rlen(&p[1], n);
        if (len == 0 || (len + 1 + n) > MQTT_CTL_PKT_SIZE) {
            /* invalid length */
            return E_OBJ;
        }
        
        /* 4 bytes already read, adjust remaining len to read */
        if (n < 4) {
            len = len - (4 - n); /* required len to read */
            n = 4;
        }
    }
    else {
        n = 1;
        len = p[1] & 0x7F;
    }
    p = p + 1 + n;

    /* read remaining bytes (variable header + payload) */
    ercd = mqtt_tcp_recv(s, p, len);
    if (ercd != E_OK) {
        return ercd;
    }
    if (n == 1)
        *pp_varhdr = p;

    p_msg->pkt_len = 1 + n + len;
    //p_msg->pkt_dat = p[1 + n];
    return E_OK;
}

static ER mqtt_rx_process(T_MQTT_CLIENT *p_cli)
{
    T_MQTT_MSG  *p_msg, *p_waimsg;
    ER ercd;
    UB pkt_type, qos;
    UB *p_varhdr;
    UH len;
    ENUM_MQTT_SESSION_STATE sess_state;

    /* session established ? */
    sess_state = mqtt_get_state(p_cli);
    if ((sess_state == MQTT_SESS_NONE) ||
        (sess_state == MQTT_SESS_CLOSED)) {
        return E_OBJ;
    }

    /* allocate buffer for receive packet */
    ercd = mqtt_get_rcv_buf(p_cli, &p_msg, TMO_POL);
    if (ercd != E_OK) {
        if (ercd == E_TMOUT) {
            mqtt_ulc(p_cli);
            dly_tsk(10);    /* allow execution of other tasks */
            mqtt_loc(p_cli);
        }
        return ercd;
    }

    /* wait and receive packet */
    ercd = mqtt_rcv_pkt(p_cli, p_msg, &p_varhdr, TMO_POL);
    if (ercd != E_OK) {
        mqtt_ret_rcv_buf(p_msg);
        return E_CLS;
    }

    /* rcv cmd ? add to receive queue
     * rcv ack ? search waiting command and complete send process
     * other ? drop
     */
    ercd = E_OK;

    /* validate for minimum required length and packet identifier */
    pkt_type = MQTT_GET_PKT_TYPE(p_msg->pkt[0]);
    switch (pkt_type) {
        case MQTT_PKT_TYPE_PUBLISH:
            /* validate QoS */
            qos = MQTT_GET_PKT_QOS(p_msg->pkt[0]);
            if (qos > 2) {
                pkt_type = MQTT_PKT_TYPE_NONE;  /* Invalid QoS value */
                break;
            }
            /* validate Topic name syntax */
            if (p_msg->pkt_len < 4) {
                pkt_type = MQTT_PKT_TYPE_NONE;  /* Invalid length */
                break;
            }
            len = MQTT_GET_UH(p_varhdr); /* Topic len */
            if (p_msg->pkt_len < (4 + len)) {
                pkt_type = MQTT_PKT_TYPE_NONE;  /* Invalid length */
                break;
            }
            if(!(mqtt_chk_pub_topic(p_varhdr+2, len))) {
                pkt_type = MQTT_PKT_TYPE_NONE;  /* Invalid Topic name */
                break;
            }
            /* validate packet identifier */
            if (qos > 0) {
                if (p_msg->pkt_len < (4+len+2)) {
                    pkt_type = MQTT_PKT_TYPE_NONE;  /* Invalid length */
                    break;
                }
                p_msg->pkt_id = MQTT_GET_PKT_ID(p_varhdr+2+len);
                if (p_msg->pkt_id == 0) {
                    pkt_type = MQTT_PKT_TYPE_NONE;  /* Invalid Packet Id */
                    break;
                }
            }
            break;
        case MQTT_PKT_TYPE_PUBREL:
        case MQTT_PKT_TYPE_PUBACK:
        case MQTT_PKT_TYPE_PUBREC:
        case MQTT_PKT_TYPE_PUBCOMP:
        case MQTT_PKT_TYPE_SUBACK:
        case MQTT_PKT_TYPE_UNSUBACK:
            if (p_msg->pkt_len < 3) {
                pkt_type = MQTT_PKT_TYPE_NONE;  /* Invalid length */
                break;
            }
            p_msg->pkt_id = MQTT_GET_PKT_ID(p_varhdr);
            if (p_msg->pkt_id == 0) {
                pkt_type = MQTT_PKT_TYPE_NONE;  /* Invalid Packet Id */
                break;
            }
            break;
        case MQTT_PKT_TYPE_CONNACK:
            if (p_msg->pkt_len < 3) {
                pkt_type = MQTT_PKT_TYPE_NONE;  /* Invalid length */
                break;
            }
            if ((p_varhdr[0] & 0xFE) != 0) {
                pkt_type = MQTT_PKT_TYPE_NONE;  /* Invalid reserved bit */
            }
            break;
        default:
            break;
    }

    /* process received message */
    switch (pkt_type) {
        case MQTT_PKT_TYPE_PUBLISH:
             p_msg->state = MQTT_MSG_STATE_RX_CMD;
             /* put in subscribe mail box */
             mqtt_que_pub_msg(p_cli, p_msg);
             p_msg = NULL;
             break;
        case MQTT_PKT_TYPE_PUBREL:
            p_msg->state = MQTT_MSG_STATE_RX_CMD;
            /* put in subscribe mail box */
            mqtt_que_pub_msg(p_cli, p_msg);
            p_msg = NULL;
            break;
        case MQTT_PKT_TYPE_CONNACK:
            p_waimsg = mqtt_fnd_wait_que_msg(p_cli, MQTT_PKT_TYPE_CONNECT, FALSE, 0);
            if (p_waimsg) {
                p_waimsg->p_pkt2 = p_msg;
                p_msg = NULL;
                mqtt_snd_msg_done(p_waimsg, E_OK);
            }
            break;
        case MQTT_PKT_TYPE_PUBACK: /* no break */
        case MQTT_PKT_TYPE_PUBREC:
            p_waimsg = mqtt_fnd_wait_que_msg(p_cli, MQTT_PKT_TYPE_PUBLISH, TRUE, p_msg->pkt_id);
            if (p_waimsg) {
                mqtt_snd_msg_done(p_waimsg, E_OK);
            }
            break;
        case MQTT_PKT_TYPE_PUBCOMP:
            p_waimsg =mqtt_fnd_wait_que_msg(p_cli, MQTT_PKT_TYPE_PUBREL, TRUE, p_msg->pkt_id);
            if (p_waimsg) {
                mqtt_snd_msg_done(p_waimsg, E_OK);
            }
            break;
        case MQTT_PKT_TYPE_SUBACK:
            p_waimsg = mqtt_fnd_wait_que_msg(p_cli, MQTT_PKT_TYPE_SUBSCRIBE, TRUE, p_msg->pkt_id);
            if (p_waimsg) {
                p_waimsg->p_pkt2 = p_msg;
                p_msg = NULL;
                mqtt_snd_msg_done(p_waimsg, E_OK);
            }
            break;
        case MQTT_PKT_TYPE_UNSUBACK:
            p_waimsg = mqtt_fnd_wait_que_msg(p_cli, MQTT_PKT_TYPE_UNSUBSCRIBE, TRUE, p_msg->pkt_id);
            if (p_waimsg) {
                mqtt_snd_msg_done(p_waimsg, E_OK);
            }
            break;
        case MQTT_PKT_TYPE_PINGRESP:
            p_waimsg = mqtt_fnd_wait_que_msg(p_cli, MQTT_PKT_TYPE_PINGREQ, FALSE, 0);
            if (p_waimsg) {
                mqtt_snd_msg_done(p_waimsg, E_OK);
            }
            break;
        case MQTT_PKT_TYPE_CONNECT:
        case MQTT_PKT_TYPE_SUBSCRIBE:
        case MQTT_PKT_TYPE_UNSUBSCRIBE:
        case MQTT_PKT_TYPE_PINGREQ:
        case MQTT_PKT_TYPE_DISCONNECT:
        default:
            ercd = E_CLS;
            break;
    }

    if (p_msg) {
        mqtt_ret_rcv_buf(p_msg);
        p_msg = NULL;
    }

    return ercd;
}

void mqtt_rcv_tsk_process(T_MQTT_CLIENT *p_cli)
{
    FLGPTN  flgptn;
    ER ercd;
    ENUM_MQTT_SESSION_STATE sess_state;

    if (p_cli == NULL) {
        return;
    }

    while (1) {

        ercd = wai_flg(p_cli->evt_flgid, MQTT_EV_CON, TWF_ORW, &flgptn);
        if (ercd != E_OK) {
            return;
        }

        mqtt_loc(p_cli);

        sess_state = mqtt_get_state(p_cli);
        if (sess_state == MQTT_SESS_NONE) {
            mqtt_ulc(p_cli);
            return; /* Client deleted */
        }

        /*clr_flg(p_cli->evt_flgid, ~(flgptn & MQTT_EV_CON));*/
        for (;;) {
            ercd = mqtt_rx_process(p_cli);
            if (ercd != E_OK && ercd != E_TMOUT) {
                break;
            }
        }
        /* Should not call if mqtt_cls() is doing */
        if (mqtt_get_state(p_cli) != MQTT_SESS_CLOSING) {
            /* close TCP connection */
            mqtt_tcp_cls(p_cli, TMO_POL);
        }

        mqtt_ulc(p_cli);
    }
}

void mqtt_tx_process(T_MQTT_CLIENT *p_cli)
{
    T_MQTT_MSG  *p_msg;
    ER ercd;
    UB pkt_type;

    for (;;) {

        /* wait for transmit packet (command or Ack) */
        p_msg = mqtt_get_que(p_cli, &p_cli->tx_que);
        if (p_msg == NULL) {
            break;
        }
        p_msg->state = MQTT_MSG_STATE_TRANSMITTING;

        /* transmit the packet to MQTT server */
        ercd = mqtt_tcp_send(p_cli, p_msg->pkt, p_msg->pkt_len);

        if (p_msg->state == MQTT_MSG_STATE_ABORT) {
            mqtt_ret_snd_buf(p_msg);
            continue;
        }

        if (ercd != E_OK) {
            p_msg->cause = p_cli->use_ssl ? MQTT_ERR_TLS_SND : MQTT_ERR_TCP_SND;
            mqtt_snd_msg_done(p_msg, E_CLS);
            continue;
        }

        pkt_type = (p_msg->pkt[0] & 0xF0) >> 4;
        switch (pkt_type) {
            case MQTT_PKT_TYPE_CONNECT:
            case MQTT_PKT_TYPE_SUBSCRIBE:
            case MQTT_PKT_TYPE_UNSUBSCRIBE:
            case MQTT_PKT_TYPE_PINGREQ:
            case MQTT_PKT_TYPE_PUBREL:
                /* add to wait queue */
                p_msg->state = MQTT_MSG_STATE_TX_WAIT_ACK;
                mqtt_add_que(p_cli, &p_cli->tx_ack_wai_que, p_msg);
                break;
            case MQTT_PKT_TYPE_PUBLISH:
                if (MQTT_GET_PKT_QOS(p_msg->pkt[0]) == 0) {
                    p_msg->state = MQTT_MSG_STATE_DONE;
                    mqtt_snd_msg_done(p_msg, E_OK);
                }
                else {
                    /* add to wait queue */
                    p_msg->state = MQTT_MSG_STATE_TX_WAIT_ACK;
                    mqtt_add_que(p_cli, &p_cli->tx_ack_wai_que, p_msg);
                }
                break;
            case MQTT_PKT_TYPE_DISCONNECT:
                mqtt_snd_msg_done(p_msg, E_OK);
                break;

            case MQTT_PKT_TYPE_CONNACK:
            case MQTT_PKT_TYPE_PUBACK:
            case MQTT_PKT_TYPE_PUBREC:
            case MQTT_PKT_TYPE_PUBCOMP:
            case MQTT_PKT_TYPE_SUBACK:
            case MQTT_PKT_TYPE_UNSUBACK:
            case MQTT_PKT_TYPE_PINGRESP:
                /* complete, release packet */
                p_msg->state = MQTT_MSG_STATE_DONE;
                mqtt_ret_rcv_buf(p_msg);
                break;

            default:
                mqtt_ret_rcv_buf(p_msg);
                break;
        }
    }
}

void mqtt_snd_tsk_process(T_MQTT_CLIENT *p_cli)
{
    FLGPTN  flgptn;
    ER ercd;
    ENUM_MQTT_SESSION_STATE sess_state;

    if (p_cli == NULL) {
        return;
    }

    while (1) {

        ercd = wai_flg(p_cli->evt_flgid, MQTT_EV_TX_RDY, TWF_ORW, &flgptn);
        if (ercd != E_OK) {
            return;
        }

        mqtt_loc(p_cli);

        sess_state = mqtt_get_state(p_cli);
        if (sess_state == MQTT_SESS_NONE) {
            mqtt_ulc(p_cli);
            return; /* Client deleted */
        }

        /*clr_flg(p_cli->evt_flgid, ~(flgptn & MQTT_EV_TX_RDY));*/

        mqtt_tx_process(p_cli);

        mqtt_ulc(p_cli);
    }
}

ER mqtt_snd(T_MQTT_MSG *p_msg, TMO tmo)
{
    T_MQTT_CLIENT *p_cli;
    ER ercd;

    p_cli = p_msg->p_cli;

    get_tid(&p_msg->wai_tskid);
    can_wup(TSK_SELF);

    p_msg->wai_flg = TRUE;
    p_msg->state = MQTT_MSG_STATE_TX_PENDING;
    ercd = add_mqtt_lst(&p_cli->tx_que, (T_MQTT_LST_ELE *)p_msg, LST_BOTTOM);

    if (ercd != E_OK) {
        p_msg->state = MQTT_MSG_STATE_ERROR;
        p_msg->ercd  = ercd;
        p_msg->cause = MQTT_ERR_INTERNAL;
        return ercd;
    }

    set_flg(p_cli->evt_flgid, MQTT_EV_TX_RDY);

    mqtt_ulc(p_cli);
    ercd = tslp_tsk(tmo);
    mqtt_loc(p_cli);

    p_msg->wai_flg   = FALSE;
    can_wup(TSK_SELF);

    /* check message status */

    switch (p_msg->state) {
        case MQTT_MSG_STATE_DONE:
        case MQTT_MSG_STATE_ERROR:
            ercd = p_msg->ercd;
            break;
        case MQTT_MSG_STATE_TX_PENDING:
            rmv_mqtt_lst(&p_cli->tx_que, (T_MQTT_LST_ELE *)p_msg);
            p_msg->cause = MQTT_ERR_CMD_TIMOUT;
            ercd = E_TMOUT;
            break;
        case MQTT_MSG_STATE_TX_WAIT_ACK:
            rmv_mqtt_lst(&p_cli->tx_ack_wai_que, (T_MQTT_LST_ELE *)p_msg);
            p_msg->cause = MQTT_ERR_CMD_UNRES;
            ercd = E_TMOUT;
            break;
        case MQTT_MSG_STATE_TRANSMITTING:
            /* in transmission, could not remove from queue */
            /* mark as removed */
            p_msg->state = MQTT_MSG_STATE_ABORT;
            p_msg->cause = MQTT_ERR_CMD_TIMOUT;
            ercd = E_TMOUT;
            break;
        default:
            break;
    }

    mqtt_ulc(p_cli);

    return ercd;
}

void mqtt_snd_wait_wakeup(T_MQTT_MSG *p_msg)
{
    if (p_msg != NULL) {
        if (p_msg->wai_flg) {
            p_msg->wai_flg = FALSE;
            wup_tsk(p_msg->wai_tskid);
        }
    }
}

T_MQTT_MSG *mqtt_rcv(T_MQTT_CLIENT *p_cli)
{
    T_MQTT_MSG *p_msg;

    p_msg = (T_MQTT_MSG *)ref_mqtt_lst(&p_cli->rx_que, LST_TOP);
    if (p_msg) {
        rmv_mqtt_lst(&p_cli->rx_que, (T_MQTT_LST_ELE *)p_msg);
    }

    return p_msg;
}

ER mqtt_rcv_done(T_MQTT_MSG *p_msg)
{
    T_MQTT_CLIENT *p_cli;
    ER ercd;
    UB pkt_type, qos;

    if (p_msg == NULL || p_msg->p_cli == NULL) {
        return E_PAR;
    }
    p_cli = p_msg->p_cli;

    ercd = E_OK;
    /* send ACK if it is publish packet */
    pkt_type = MQTT_GET_PKT_TYPE(p_msg->pkt[0]);
    switch (pkt_type) {
        case MQTT_PKT_TYPE_PUBLISH:
            /* construct ACK */
            qos = MQTT_GET_PKT_QOS(p_msg->pkt[0]);
            if (qos == 2)
                mqtt_bld_pub_res(p_msg, MQTT_PKT_TYPE_PUBREC);
            else if (qos == 1)
                mqtt_bld_pub_res(p_msg, MQTT_PKT_TYPE_PUBACK);
            else {
                mqtt_ret_rcv_buf(p_msg);
                break;
            }
            /* add to transmit queue */
            mqtt_que_snd_msg(p_cli, p_msg);
            break;

        case MQTT_PKT_TYPE_PUBREL:
            /* construct ACK */
            mqtt_bld_pub_res(p_msg, MQTT_PKT_TYPE_PUBCOMP);
            /* add to transmit queue */
            mqtt_que_snd_msg(p_cli, p_msg);
            break;
        default:
            /* release */
            mqtt_ret_rcv_buf(p_msg);
            break;
    }

    return ercd;
}

/* mqtt_cls_que: called from mqtt_cls() to clear tx/rx queue messages,
   It should be called after mqtt_tcp_cls().
 */

void mqtt_cls_que(T_MQTT_CLIENT *p_cli)
{
    T_MQTT_MSG *p_msg;

    /* Clear 'rx_que' messages */
    for (;;) {
        p_msg = mqtt_get_que(p_cli, &p_cli->rx_que);
        if (p_msg == NULL) {
            break;
        }
        mqtt_ret_rcv_buf(p_msg);
    }

    /* Clear message delivered from 'rx_que' */
    /* Message locked in p_cli->cbk() ? , mark it for abort */
    p_msg = p_cli->p_cbk_msg;
    if (p_msg != NULL) {
        p_msg->state = MQTT_MSG_STATE_ABORT;
        p_msg->ercd  = E_RLWAI;
    }

    /* Clear 'tx_ack_wai_que' messages. */
    for (;;) {
        p_msg = mqtt_get_que(p_cli, &p_cli->tx_ack_wai_que);
        if (p_msg == NULL) {
            break;
        }
        p_msg->cause = MQTT_ERR_CMD_CANCEL;
        mqtt_snd_msg_done(p_msg, E_RLWAI);
    }

    /* Clear 'tx_que' messages */
    /* mqtt_tx_process() would clear the message upon socket error */
    for (;;) {
        p_msg = (T_MQTT_MSG *)ref_mqtt_lst(&p_cli->tx_que, LST_TOP);
        if (p_msg == NULL) {
            break;
        }
        /* tx_que not empty, wait until mqtt_tx_process() complete */
        mqtt_ulc(p_cli);
        tslp_tsk(10); /* little delay */
        mqtt_loc(p_cli);
    }
}
