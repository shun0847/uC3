/***********************************************************************
    MICRO C CUBE / COMPACT,
     MQTT Sample Application
    Copyright (c) 2016-2022, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.08.01: Created.
      2016.12.15: Bug fixing in mqtt_sub()
                   Bug
                    - Return E_CLS instead of E_OK when QoS = 1
                    - Return E_OK instead of E_CLS when receive suscription
                      acknowledge with error.
      2016.12.19: Improved so as to save the SSL session ID.
      2017.02.13: Allowed SSL connection before mqtt_con() execution.
      2018.05.15: Bug fixing
                    - cls_soc( ) are called with bad 2nd argument.
      2018.05.15: Code Changed.
                  Change cls_soc() timeout with mqtt_cls( ) argument tmo.
      2018.05.15: Code changed.
                  mqtt_cls( ) does return cls_soc( ) return value, If other
                  codes are success.
      2018.05.15: Type of socket id are changed ID to SID.
      2022.04.25: Supported MQTTS connection using uNet3-TLS.
      2022.10.25: Bug fixing
                    - Double release of TX buffer in API.
      2022.12.14: Added error informatoins.
************************************************************************/

#include <string.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_strlib.h"
#include "mqtt_util.h"
#include "mqtt.h"
#include "mqtt_msg.h"
#include "mqtt_cfg.h"
#ifdef MQTT_SSL_SUP
#include "ssl_hdr.h"
#endif

#ifdef MQTT_SSL_SUP
#ifdef SSL_SERVER_NODE  /* using uNet3-TLS */
static ER mqtt_con_ssoc(SID sid, T_NODE *host, ID ssnid, TMO tmo)
{
    T_SSL_NODE sn = {0};
    sn.node_type = SSL_CLIENT_NODE;
    sn.host = *host;
    return con_ssoc(sid, &sn, ssnid, tmo);
}
#else
#define mqtt_con_ssoc  con_ssoc
#endif
#endif

ER mqtt_tcp_con(T_MQTT_CLIENT *p_cli, T_NODE *host, BOOL use_ssl, TMO tmo, T_MQTT_MSG *msg)
{
    T_NODE h;
    SID sid;
    ER ercd;

    sid = p_cli->soc_id;

    net_memcpy((VB *)&h, host, sizeof(h));
    if (h.port == 0) {
        if (use_ssl) {
            h.port = MQTT_SSL_SERVER_PORT;
        }
        else {
            h.port = MQTT_SERVER_PORT;
        }
    }

    ercd = cfg_soc(sid, SOC_TMO_CON, (VP)tmo);
    if (ercd != E_OK) {
        msg->cause = MQTT_ERR_SOCKET;
        return ercd;
    }

    mqtt_ulc(p_cli);

    p_cli->ssn_id = 0;
    if (use_ssl) {
#ifdef MQTT_SSL_SUP
        ercd = get_ssid_soc(sid);
        if (ercd == 0) {
            ercd = mqtt_con_ssoc(sid, &h, 0, tmo);
        }
        if (ercd > 0) {
            p_cli->ssn_id = ercd;
            ercd = E_OK;
        } else {
            msg->cause = MQTT_ERR_TLS_CON;
        }
#else
        ercd = E_NOSPT;
#endif
    }
    else {
        ercd = con_soc(sid, &h, SOC_CLI);
        if (ercd < E_OK) {
            msg->cause = MQTT_ERR_TCP_CON;
        }
    }

    mqtt_loc(p_cli);

    return ercd;
}

ER mqtt_tcp_cls(T_MQTT_CLIENT *p_cli, TMO tmo)
{
    SID sid;
    ER ercd;

    sid = p_cli->soc_id;

    mqtt_ulc(p_cli);
    
    (void)cfg_soc(sid, SOC_TMO_CLS, (VP)tmo);

    if (p_cli->use_ssl) {
#ifdef MQTT_SSL_SUP
        ercd = cls_ssoc(sid);
#else
        ercd = E_NOSPT;
#endif
    }
    else {
        ercd = cls_soc(sid, SOC_TCP_CLS);
    }

    mqtt_loc(p_cli);

    return ercd;
}

/*
  Transmit 'len' bytes of data to network (to MQTT Server)
  E_OK  - Transmitted 'len' bytes of data.
  E_CLS - Network error, transmission incomplete.
*/
ER mqtt_tcp_send(T_MQTT_CLIENT *p_cli, UB *data, UH len)
{
    SID sid;
    ER ercd;
    UH i;
    UB use_ssl;

    sid = p_cli->soc_id;
    use_ssl = p_cli->use_ssl;

    mqtt_ulc(p_cli);

    i = 0;
    while (len > 0) {
        if (use_ssl) {
#ifdef MQTT_SSL_SUP
            ercd = snd_ssoc(sid, data + i, len);
#else
            ercd = E_NOSPT;
#endif
        }
        else {
            ercd = snd_soc(sid, data + i, len);
        }
        if (ercd <= 0) {
            break;
        }
        i += (UH)ercd;
        len -= (UH)ercd;
    }

    if (len == 0)
        ercd = E_OK;
    else
        ercd = E_CLS;

    mqtt_loc(p_cli);

    return ercd;
}

/*
  Receive 'len' bytes of data from network (from MQTT Server)
  E_OK  - Received 'len' bytes of data.
  E_CLS - Network error, reception incomplete.
*/
ER mqtt_tcp_recv(T_MQTT_CLIENT *p_cli, UB *data, UH len)
{
    SID sid;
    ER ercd;
    UH i;
    UB use_ssl;

    sid = p_cli->soc_id;
    use_ssl = p_cli->use_ssl;

    mqtt_ulc(p_cli);

    i = 0;
    while (len > 0)
    {
        if (use_ssl) {
#ifdef MQTT_SSL_SUP
            ercd = rcv_ssoc(sid, data + i, len);
#else
            ercd = E_NOSPT;
#endif
        }
        else {
            ercd = rcv_soc(sid, data + i, len);
        }
        if (ercd <= 0) {
            break;
        }
        i   += (UH)ercd;
        len -= (UH)ercd;
    }

    if (len == 0)
        ercd = E_OK;
    else
        ercd = E_CLS;

    mqtt_loc(p_cli);

    return ercd;
}

ER mqtt_cli_cre(T_MQTT_CLIENT *p_cli)
{
    if (p_cli->soc_id == 0)
        return E_ID;
    if (p_cli->loc_semid == 0)
        return E_ID;
    if (p_cli->evt_flgid == 0)
        return E_ID;
    if (p_cli->tx_mpfid == 0)
        return E_ID;
    if (p_cli->rx_mpfid == 0)
        return E_ID;
    if (p_cli->tx_tskid == 0)
        return E_ID;
    if (p_cli->rx_tskid == 0)
        return E_ID;
    if (p_cli->cbk == NULL)
        return E_ID;
    if (p_cli->max_msg_size < MQTT_MIN_PKT_SIZE)
        return E_PAR;

    mqtt_loc(p_cli);
    clr_flg(p_cli->evt_flgid, 0);
    ini_mqtt_lst(&p_cli->tx_que);
    ini_mqtt_lst(&p_cli->tx_ack_wai_que);
    ini_mqtt_lst(&p_cli->rx_que);
    p_cli->p_cbk_msg = NULL;

    p_cli->state = MQTT_SESS_CLOSED;

    sta_tsk(p_cli->tx_tskid, (VP_INT)p_cli);
    sta_tsk(p_cli->rx_tskid, (VP_INT)p_cli);
    sta_tsk(p_cli->rx_app_tskid, (VP_INT)p_cli);

    mqtt_ulc(p_cli);

    return E_OK;
}

ER mqtt_cli_del(T_MQTT_CLIENT *p_cli)
{
    if (p_cli == NULL) {
        return E_PAR;
    }

    mqtt_loc(p_cli);

    /* Should not delete if session is active */
    if (mqtt_get_state(p_cli) != MQTT_SESS_CLOSED) {
        mqtt_ulc(p_cli);
        return E_OBJ;
    }

    /* Set state to none and set flags to terminate tasks */
    mqtt_set_state(p_cli, MQTT_SESS_NONE);
    set_flg(p_cli->evt_flgid, MQTT_EV_CON);    /* mqtt_rcv_tsk */
    set_flg(p_cli->evt_flgid, MQTT_EV_TX_RDY); /* mqtt_snd_tsk */
    set_flg(p_cli->evt_flgid, MQTT_EV_RX_RDY); /* mqtt_apprcv_tsk */

    mqtt_ulc(p_cli);

    tslp_tsk(10); /* little delay to allow task to complete */

    ter_tsk(p_cli->tx_tskid);
    ter_tsk(p_cli->rx_tskid);
    ter_tsk(p_cli->rx_app_tskid);

    return E_OK;
}

ER mqtt_con(T_MQTT_CLIENT *p_cli, T_MQTT_CON *p_con, TMO tmo)
{
    T_MQTT_CON_ACK con_ack;
    T_MQTT_MSG *msg;
    ER ercd;

    if (p_cli == NULL || p_con == NULL) {
        return E_PAR;
    }

    p_con->cause = MQTT_ERR_NONE;

    /* 1. Allocate buffer */
    ercd = mqtt_get_snd_buf(p_cli, &msg, tmo);
    if (ercd != E_OK) {
        p_con->cause = MQTT_ERR_NOMEM;
        return ercd;
    }

    mqtt_loc(p_cli);

    if (mqtt_get_state(p_cli) != MQTT_SESS_CLOSED) {
        p_con->cause = MQTT_ERR_NOT_CLOSE;
        mqtt_ret_snd_buf(msg);
        mqtt_ulc(p_cli);
        return E_OBJ;
    }

    /* Initialize return code, this is required as same E_CLS
     * return code is used for both TCP close and protocol close */
    p_con->return_code = 0;

    /* 2. Construct CONNECT command */
    ercd = mqtt_enc_con(msg, p_con);
    if (ercd != E_OK) {
        p_con->cause = msg->cause;
        mqtt_ret_snd_buf(msg);
        mqtt_ulc(p_cli);
        return ercd;
    }

    mqtt_set_state(p_cli, MQTT_SESS_CONNECTING);

    p_cli->use_ssl = p_con->use_ssl;

    /* 3. Establish TCP connection with MQTT broker */
    ercd = mqtt_tcp_con(p_cli, &p_con->broker, p_con->use_ssl, tmo, msg);
    if (ercd != E_OK) {
        goto mqtt_con_err;
    }
    set_flg(p_cli->evt_flgid, MQTT_EV_CON);

    /* 4. Issue command and wait for CONNACK */
    ercd = mqtt_snd(msg, tmo);
    if (ercd != E_OK) {
        goto mqtt_con_err;
    }

    /* 5. Decode CONNACK */
    ercd = mqtt_dec_con_ack(msg, &con_ack);
    if (ercd != E_OK) {
        goto mqtt_con_err;
    }

    p_con->sp = con_ack.sp;
    p_con->return_code = con_ack.return_code;

    if (con_ack.return_code != 0) {
        msg->cause = MQTT_ERR_CMD_ERRRES;
        ercd = E_CLS;
        goto mqtt_con_err;
    }

    /* 6. Set connection state */
    mqtt_set_state(p_cli, MQTT_SESS_CONNECTED);

    p_con->cause = msg->cause;
    mqtt_ret_snd_buf(msg);
    return ercd;

mqtt_con_err:
    p_con->cause = msg->cause;
    mqtt_set_state(p_cli, MQTT_SESS_CLOSED);
    if (msg->state != MQTT_MSG_STATE_ABORT) {
        mqtt_ret_snd_buf(msg);
    }
    mqtt_tcp_cls(p_cli, tmo);
    mqtt_ulc(p_cli);
    return ercd;
}

ER mqtt_pub(T_MQTT_CLIENT *p_cli, T_MQTT_PUB *p_pub, TMO tmo)
{
    T_MQTT_MSG *msg;
    ER ercd;
    UB cmd;

    if (p_cli == NULL || p_pub == NULL) {
        return E_PAR;
    }

    p_pub->cause = MQTT_ERR_NONE;

    /* 1. Allocate buffer */

    ercd = mqtt_get_snd_buf(p_cli, &msg, tmo);
    if (ercd != E_OK) {
        p_pub->cause = MQTT_ERR_NOMEM;
        return ercd;
    }

    mqtt_loc(p_cli);

    if (mqtt_get_state(p_cli) != MQTT_SESS_CONNECTED) {
        p_pub->cause = MQTT_ERR_NOT_CONNECT;
        mqtt_ret_snd_buf(msg);
        mqtt_ulc(p_cli);
        return E_OBJ;
    }

    cmd = p_pub->cmd;
    if (!((cmd == MQTT_PKT_TYPE_NONE) || (cmd == MQTT_PKT_TYPE_PUBLISH) ||
        ((cmd == MQTT_PKT_TYPE_PUBREL) && (p_pub->qos == 2)))) {
        p_pub->cause = MQTT_ERR_INV_QOS;
        mqtt_ret_snd_buf(msg);
        mqtt_ulc(p_cli);
        return E_PAR;
    }
    p_pub->cmd = MQTT_PKT_TYPE_NONE;

    if (cmd == MQTT_PKT_TYPE_PUBREL) {
        msg->state = MQTT_MSG_STATE_DONE; /* for mqtt_enc_pubrel() */
        goto mqtt_snd_pubrel;
    }

    p_pub->cmd = MQTT_PKT_TYPE_PUBLISH;
    /* 2. Construct PUBLISH command */
    ercd = mqtt_enc_pub(msg, p_pub);
    if (ercd != E_OK) {
        goto mqtt_pub_err;
    }
    if (p_pub->pkt_id == 0)
        p_pub->pkt_id = msg->pkt_id;

    /* 3. Issue command and wait for response */
    /*    For QoS 0 - return after command transmission */
    /*        Qos 1 - return after receive PUBACK     */
    /*        Qos 2 - return after receive PUBREC     */
    ercd = mqtt_snd(msg, tmo);
    if (ercd != E_OK) {
        goto mqtt_pub_err;
    }

    /* 4. Complete command */
    /*    For QoS 0 - This step not applicable. */
    /*        Qos 1 - This step not applicable. */
    /*        Qos 2 - Send PUBREL */
mqtt_snd_pubrel:
    if (p_pub->qos == 2) {
        p_pub->cmd = MQTT_PKT_TYPE_PUBREL;

        /* Send PUBREL command */
        ercd = mqtt_enc_pubrel(msg, p_pub->pkt_id);
        if (ercd != E_OK) {
            goto mqtt_pub_err;
        }

        /* Issue command and wait for PUBCOMP */
        ercd = mqtt_snd(msg, tmo);
        if (ercd != E_OK) {
            goto mqtt_pub_err;
        }
    }

mqtt_pub_err:
    p_pub->cause = msg->cause;
    if (msg->state != MQTT_MSG_STATE_ABORT) {
        mqtt_ret_snd_buf(msg);
    }
    mqtt_ulc(p_cli);
    return ercd;
}

ER mqtt_sub(T_MQTT_CLIENT *p_cli, T_MQTT_SUB *p_sub, TMO tmo)
{
    T_MQTT_SUB_TOPIC *topic;
    T_MQTT_SUB_ACK sub_ack;
    T_MQTT_MSG *msg;
    ER ercd;
    UH i;

    if (p_cli == NULL || p_sub == NULL) {
        return E_PAR;
    }

    p_sub->cause = MQTT_ERR_NONE;

    /* 1. Allocate buffer */
    ercd = mqtt_get_snd_buf(p_cli, &msg, tmo);
    if (ercd != E_OK) {
        p_sub->cause = MQTT_ERR_NOMEM;
        return ercd;
    }

    mqtt_loc(p_cli);

    if (mqtt_get_state(p_cli) != MQTT_SESS_CONNECTED) {
        p_sub->cause = MQTT_ERR_NOT_CONNECT;
        mqtt_ret_snd_buf(msg);
        mqtt_ulc(p_cli);
        return E_OBJ;
    }

    /* Initialize return code */
    topic = p_sub->p_topic_list;
    for (i=0;i<p_sub->topic_count;i++) {
        topic[i].return_code = 0;
    }

    /* 2. Construct SUBSCRIBE command */
    ercd = mqtt_enc_sub(msg, p_sub);
    if (ercd != E_OK) {
        goto mqtt_sub_err;
    }

    /* 3. Issue command and wait for SUBACK */
    ercd = mqtt_snd(msg, tmo);
    if (ercd != E_OK) {
        goto mqtt_sub_err;
    }

    /* 4. Verify received SUBACK */
    ercd = mqtt_dec_sub_ack(msg, &sub_ack);
    if (ercd != E_OK) {
        goto mqtt_sub_err;
    }

    /* 5. return E_CLS if any topic return code is not success */
    if (sub_ack.topic_count != p_sub->topic_count) {
        msg->cause = MQTT_ERR_CMD_ERRRES;
        ercd = E_CLS;
    }

    topic = p_sub->p_topic_list;
    for (i=0;i<sub_ack.topic_count;i++) {
        topic[i].return_code = sub_ack.p_return_code[i];
        if ((sub_ack.p_return_code[i] & 0x80) == 0x80) {  /* sub error */
            msg->cause = MQTT_ERR_CMD_ERRRES;
            ercd = E_CLS;
        }
    }

mqtt_sub_err:
    p_sub->cause = msg->cause;
    if (msg->state != MQTT_MSG_STATE_ABORT) {
        mqtt_ret_snd_buf(msg);
    }
    mqtt_ulc(p_cli);
    return ercd;
}

ER mqtt_unsub(T_MQTT_CLIENT *p_cli, T_MQTT_UNSUB *p_unsub, TMO tmo)
{
    T_MQTT_MSG *msg;
    ER ercd;

    if (p_cli == NULL || p_unsub == NULL) {
        return E_PAR;
    }

    p_unsub->cause = MQTT_ERR_NONE;

    /* 1. Allocate buffer */
    ercd = mqtt_get_snd_buf(p_cli, &msg, tmo);
    if (ercd != E_OK) {
        p_unsub->cause = MQTT_ERR_NOMEM;
        return ercd;
    }

    mqtt_loc(p_cli);

    if (mqtt_get_state(p_cli) != MQTT_SESS_CONNECTED) {
        p_unsub->cause = MQTT_ERR_NOT_CONNECT;
        mqtt_ret_snd_buf(msg);
        mqtt_ulc(p_cli);
        return E_OBJ;
    }

    /* 2. Construct UNSUBSCRIBE command */
    ercd = mqtt_enc_unsub(msg, p_unsub);
    if (ercd != E_OK) {
        goto mqtt_unsub_err;
    }

    /* 3. Issue command and wait for SUBACK */
    ercd = mqtt_snd(msg, tmo);
    if (ercd != E_OK) {
        goto mqtt_unsub_err;
    }

mqtt_unsub_err:
    p_unsub->cause = msg->cause;
    if (msg->state != MQTT_MSG_STATE_ABORT) {
        mqtt_ret_snd_buf(msg);
    }
    mqtt_ulc(p_cli);
    return ercd;
}

ER mqtt_ping(T_MQTT_CLIENT *p_cli, TMO tmo)
{
    T_MQTT_MSG *msg;
    ER ercd;

    if (p_cli == NULL) {
        return E_PAR;
    }

    /* 1. Allocate buffer */
    ercd = mqtt_get_snd_buf(p_cli, &msg, tmo);
    if (ercd != E_OK) {
        return ercd;
    }

    mqtt_loc(p_cli);

    if (mqtt_get_state(p_cli) != MQTT_SESS_CONNECTED) {
        mqtt_ret_snd_buf(msg);
        mqtt_ulc(p_cli);
        return E_OBJ;
    }

    /* 2. Construct PINGREQ command */
    ercd = mqtt_enc_ping(msg);
    if (ercd != E_OK) {
        goto mqtt_ping_err;
    }

    /* 3. Issue command and wait for PINGRES */
    ercd = mqtt_snd(msg, tmo);
    if (ercd != E_OK) {
        goto mqtt_ping_err;
    }

mqtt_ping_err:
    if (msg->state != MQTT_MSG_STATE_ABORT) {
        mqtt_ret_snd_buf(msg);
    }
    mqtt_ulc(p_cli);
    return ercd;
}

ER mqtt_cls(T_MQTT_CLIENT *p_cli, TMO tmo)
{
    T_MQTT_MSG *msg = NULL;
    ER ercd;
    ER ercd_cls;

    if (p_cli == NULL) {
        return E_PAR;
    }

    /* 1. Allocate buffer */
    ercd = mqtt_get_snd_buf(p_cli, &msg, tmo);
    if (ercd != E_OK) {
        goto mqtt_disconnect_err;
    }

    mqtt_loc(p_cli);

    if (mqtt_get_state(p_cli) == MQTT_SESS_CLOSED) {
        mqtt_ret_snd_buf(msg);
        mqtt_ulc(p_cli);
        return E_OK;
    }

    if (mqtt_get_state(p_cli) != MQTT_SESS_CONNECTED) {
        mqtt_ret_snd_buf(msg);
        mqtt_ulc(p_cli);
        return E_OBJ;   /* connect/close pending */
    }

    mqtt_set_state(p_cli, MQTT_SESS_CLOSING);

    /* 2. Construct DISCONNECT command */
    ercd = mqtt_enc_disconnect(msg);
    if (ercd != E_OK) {
        goto mqtt_disconnect_err;
    }

    /* 3. Issue command and wait for completion */
    ercd = mqtt_snd(msg, tmo);

mqtt_disconnect_err:
   if (msg->state != MQTT_MSG_STATE_ABORT) {
        mqtt_ret_snd_buf(msg);
   }
    ercd_cls = mqtt_tcp_cls(p_cli, tmo);
    if (ercd == E_OK) {
        ercd = ercd_cls;
    }
    mqtt_cls_que(p_cli);
    mqtt_set_state(p_cli, MQTT_SESS_CLOSED);
    mqtt_ulc(p_cli);
    return ercd;
}

static void mqtt_rcv_deliver(T_MQTT_CLIENT *p_cli, T_MQTT_MSG *p_msg)
{
    T_MQTT_PUB pub;
    UB ctl_type;
    ER ercd;
    BOOL res;

    res = FALSE;

    ctl_type = (p_msg->pkt[0] >> 4) & 0xF;
    switch (ctl_type)
    {
        case MQTT_PKT_TYPE_PUBLISH:
            pub.cmd    = MQTT_PKT_TYPE_PUBLISH;
            ercd = mqtt_dec_pub(p_msg, &pub);
            break;
        case MQTT_PKT_TYPE_PUBREL:
            net_memset(&pub, 0, sizeof(pub));
            pub.cmd    = MQTT_PKT_TYPE_PUBREL;
            pub.pkt_id = p_msg->pkt_id;
            ercd = E_OK;
            break;
        default:
            ercd = E_OBJ;
            break;
    }

    if (ercd == E_OK) {
        p_cli->p_cbk_msg = p_msg;   /* for cancel process */
        mqtt_ulc(p_cli);
        res = p_cli->cbk(p_cli, &pub);
        mqtt_loc(p_cli);
        p_cli->p_cbk_msg = NULL;
        if (p_msg->state == MQTT_MSG_STATE_ABORT) {
            res = FALSE; /* MQTT closing, release this message */
        }
    }

    if (res) {
        mqtt_rcv_done(p_msg);
    }
    else {
        mqtt_ret_rcv_buf(p_msg);
    }

    return;
}

void mqtt_apprcv_tsk(VP_INT exinf)
{
    T_MQTT_CLIENT  *p_cli;
    T_MQTT_MSG *p_msg;
    FLGPTN  flgptn;
    ER ercd;
    ENUM_MQTT_SESSION_STATE sess_state;

    p_cli = (T_MQTT_CLIENT *)exinf;
    if (p_cli == NULL) {
        return;
    }

    while (1) {

        ercd = wai_flg(p_cli->evt_flgid, MQTT_EV_RX_RDY, TWF_ORW, &flgptn);
        if (ercd != E_OK) {
            return;
        }

        /*clr_flg(p_cli->evt_flgid, ~(flgptn & MQTT_EV_RX_RDY));*/
        mqtt_loc(p_cli);

        sess_state = mqtt_get_state(p_cli);
        if (sess_state == MQTT_SESS_NONE) {
            mqtt_ulc(p_cli);
            return; /* Client deleted */
        }

        for (;;) {
            p_msg = mqtt_rcv(p_cli);
            if (p_msg == NULL)
                break;
            mqtt_rcv_deliver(p_cli, p_msg);
        }

        mqtt_ulc(p_cli);
    }
}

void mqtt_snd_tsk(VP_INT exinf)
{
    T_MQTT_CLIENT  *p_cli;

    p_cli = (T_MQTT_CLIENT *)exinf;
    if (p_cli == NULL) {
        return;
    }

    mqtt_snd_tsk_process(p_cli);
}

void mqtt_rcv_tsk(VP_INT exinf)
{
    T_MQTT_CLIENT  *p_cli;

    p_cli = (T_MQTT_CLIENT *)exinf;
    if (p_cli == NULL) {
        return;
    }

    mqtt_rcv_tsk_process(p_cli);
}

