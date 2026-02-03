/***************************************************************************
    MQTT Sample (Operating as command in shell)
    Copyright (c)  2025, eForce Co., Ltd. All rights reserved.

    2021/03/10: Created.
    2025/04/09: Fixed incorrect  client id length in shell_mqtt_connect().
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */

#if SAMPLE_ENA_MQTTc
#include "mqtt.h"

extern void shell_printf(VP ctrl, const char *fmt, ...) ;

extern T_MQTT_CLIENT   gMQTTClient;
extern const char mqtt_client_id[];


#if (0 == SAMPLE_USE_GENSRC)    /* no use configurator */
extern ER mqtt_usr_ini(void);
ER sample_mqttc_ini()
{
    return mqtt_usr_ini();
}
#endif
void shell_dump_char(VP ctrl, const VB *buf, UINT len)
{
    int j ;
    VB c;

    for ( j = 0 ; j < len ; j++ ) {
        c = *(buf+j) ;
        shell_printf(ctrl, "%c", c);
    }
}

static ER shell_mqtt_connect(UB use_ssl, VP ctrl, INT argc, VB *argv[])
{
    T_MQTT_CON con;
    ER ercd;

    if (argc < 3) {
        if (use_ssl)
            shell_printf(ctrl, "\r\nUsage: mqtt con_ssl ipaddress [cleansession][willtopic qos message retain] [user] [pass]");
        else
            shell_printf(ctrl, "\r\nUsage: mqtt con ipaddress [cleansession][willtopic qos message retain] [user] [pass]");
        shell_printf(ctrl, "\r\n         ipaddress   : Broker IPv4 address");
        shell_printf(ctrl, "\r\n         cleansession: 0 or 1");
        shell_printf(ctrl, "\r\n         will topic  : topic string");
        shell_printf(ctrl, "\r\n         will qos    : 0 or 1 or 2");
        shell_printf(ctrl, "\r\n         will message: message string");
        shell_printf(ctrl, "\r\n         will retain : 0 or 1");
        shell_printf(ctrl, "\r\n         user        : user name string");
        shell_printf(ctrl, "\r\n         pass        : password string");
        return E_OK;
    }

    net_memset(&con, 0, sizeof(T_MQTT_CON));
    con.broker.ipa = ip_aton(argv[2]);
    con.use_ssl = use_ssl;

    if (argc >= 4) {
        if (net_atoi(argv[3])) {
            con.clean_session = 1;
        }
    }

    if (argc >= 8) {
        con.will_topic.p_val = (UB *)argv[4];
        con.will_topic.len   = net_strlen(argv[4]);

        con.will_qos = net_atoi(argv[5]);

        con.will_message.p_val = (UB *)argv[6];
        con.will_message.len   = net_strlen(argv[6]);

        con.will_retain = net_atoi(argv[7]);
    }

    if (argc >= 9) {
        con.user.p_val = (UB *)argv[8];
        con.user.len   = net_strlen(argv[8]);
    }

    if (argc >= 10) {
        con.pass.p_val = (UB *)argv[9];
        con.pass.len   = net_strlen(argv[9]);
    }

    ercd = mqtt_cli_del(&gMQTTClient);
    shell_printf(ctrl, "\r\n  @mqtt_cli_del = %d", ercd);
    ercd = mqtt_cli_cre(&gMQTTClient);
    shell_printf(ctrl, "\r\n  @mqtt_cli_cre = %d", ercd);

    con.client_id.len = net_strlen(mqtt_client_id);
    con.client_id.p_val = (UB *)mqtt_client_id;
    ercd = mqtt_con(&gMQTTClient, &con, 10000);

    return ercd;
}

static ER shell_mqtt_publish(VP ctrl, INT argc, VB *argv[])
{
    T_MQTT_PUB pub;
    ER ercd;

    if (argc < 3) {
        shell_printf(ctrl, "\r\nUsage: mqtt pub topic [message] [qos] [retain] [dup]");
        shell_printf(ctrl, "\r\n        topic    : topic string");
        shell_printf(ctrl, "\r\n        message  : message string");
        shell_printf(ctrl, "\r\n        qos      : 0 or 1 or 2");
        shell_printf(ctrl, "\r\n        retain   : 0 or 1");
        shell_printf(ctrl, "\r\n        dup      : 0 or 1");
        return E_OK;
    }

    net_memset(&pub, 0, sizeof(T_MQTT_PUB));

    if (argc >= 3) {
        pub.topic.p_val = (UB *)argv[2];
        pub.topic.len   = net_strlen(argv[2]);
    }
    if (argc >= 4) {
        pub.p_payload   = (UB *)argv[3];
        pub.payload_len = net_strlen(argv[3]);
    }
    if (argc >= 5) {
        pub.qos = net_atoi(argv[4]);
    }
    if (argc >= 6) {
        if (net_atoi(argv[5])) {
            pub.retain = 1;
        }
    }
    if (argc >= 7) {
        if (net_atoi(argv[6])) {
            pub.dup = 1;
        }
    }

    ercd = mqtt_pub(&gMQTTClient, &pub, 10000);

    return ercd;
}

static ER shell_mqtt_subscribe(VP ctrl, INT argc, VB *argv[])
{
    T_MQTT_SUB sub;
    T_MQTT_SUB_TOPIC topic;
    ER ercd;

    if (argc < 3) {
        shell_printf(ctrl, "\r\nUsage: mqtt sub topic [qos]");
        shell_printf(ctrl, "\r\n        topic : topic string");
        shell_printf(ctrl, "\r\n        qos   : 0 or 1 or 2");
        return E_OK;
    }

    net_memset(&sub, 0, sizeof(T_MQTT_SUB));
    net_memset(&topic, 0, sizeof(T_MQTT_SUB_TOPIC));

    if (argc >= 3) {
        topic.topic.len = net_strlen(argv[2]);
        topic.topic.p_val = (UB *)argv[2];
    }
    if (argc >= 4) {
        topic.qos = net_atoi(argv[3]);
    }

    sub.topic_count = 1;
    sub.p_topic_list = &topic;

    ercd = mqtt_sub(&gMQTTClient, &sub, 10000);

    return ercd;
}

static ER shell_mqtt_unsubscribe(VP ctrl, INT argc, VB *argv[])
{
    T_MQTT_UNSUB unsub;
    T_MQTT_STR topic;
    ER ercd;

    if (argc < 3) {
        shell_printf(ctrl, "\r\nUsage: mqtt unsub topic");
        shell_printf(ctrl, "\r\n        topic : topic string");
        return E_OK;
    }

    net_memset(&unsub, 0, sizeof(T_MQTT_UNSUB));
    net_memset(&topic, 0, sizeof(T_MQTT_STR));

    if (argc >= 3) {
        topic.len = net_strlen(argv[2]);
        topic.p_val = (UB *)argv[2];
    }

    unsub.topic_count = 1;
    unsub.p_topic = &topic;

    ercd = mqtt_unsub(&gMQTTClient, &unsub, 10000);

    return ercd;
}

VP p_ctrl = NULL; // for shell printf

BOOL mqtt_rcv_callback(T_MQTT_CLIENT *p_cli, T_MQTT_PUB *p_pub)
{
    if (p_ctrl && p_pub) {

        if (p_pub->cmd == MQTT_PKT_TYPE_PUBLISH) {
            shell_printf(p_ctrl, "\r\n");
            shell_printf(p_ctrl, "Rx Publish: Identifier %d\r\n", p_pub->pkt_id);
            shell_printf(p_ctrl, "   Flags: QoS %d Retain %d Duplicate %d\r\n", p_pub->qos, p_pub->retain, p_pub->dup);
            shell_printf(p_ctrl, "   Topic: ");
            shell_dump_char(p_ctrl, (const VB *)p_pub->topic.p_val, p_pub->topic.len);
            shell_printf(p_ctrl, "\r\n");
            shell_printf(p_ctrl, "   Payload_len: %d \r\n", p_pub->payload_len);
#if 1
            shell_printf(p_ctrl, "   Payload: ");
            shell_dump_char(p_ctrl, (const VB *)p_pub->p_payload, p_pub->payload_len);
#endif
            shell_printf(p_ctrl, "\r\n");
        }
        else {
            shell_printf(p_ctrl, "\r\n");
            shell_printf(p_ctrl, "Rx PubRel: Identifier %d\r\n", p_pub->pkt_id);
        }
    }

    return TRUE;
}

ER shell_usr_cmd_mqtt(VP ctrl, INT argc, VB *argv[])
{
    ER ercd = E_OK;

    p_ctrl = ctrl;  /* used in shell_mqtt_receive() */

    if (net_strcmp(argv[1],"con") == 0) {
        ercd = shell_mqtt_connect(0, ctrl, argc, argv);
    }
    else if (net_strcmp(argv[1],"con_ssl") == 0) {
        ercd = shell_mqtt_connect(1, ctrl, argc, argv);
    }
    else if (net_strcmp(argv[1],"cls") == 0) {
        ercd = mqtt_cls(&gMQTTClient, 10000);
    }
    else if (net_strcmp(argv[1],"ping") == 0) {
        ercd = mqtt_ping(&gMQTTClient, 10000);
    }
    else if (net_strcmp(argv[1],"pub") == 0) {
        ercd = shell_mqtt_publish(ctrl, argc, argv);
    }
    else if (net_strcmp(argv[1],"sub") == 0) {
        ercd = shell_mqtt_subscribe(ctrl, argc, argv);
    }
    else if (net_strcmp(argv[1],"unsub") == 0) {
        ercd = shell_mqtt_unsubscribe(ctrl, argc, argv);
    }
    else {
        shell_printf(ctrl, "%s", "\r\n mqtt <cmd> <cmd parameters> \
                                  \r\n  cmd: \
                                  \r\n    con - connect to MQTT broker \
                                  \r\n    con_ssl - secure connect to MQTT broker");

        shell_printf(ctrl, "%s", "\r\n    pub - publish topic message \
                                  \r\n    sub - subscribe topic \
                                  \r\n    unsub - unsubscribe topic \
                                  \r\n    ping - send a ping request to broker \
                                  \r\n    cls - close connection to MQTT broker ");

#if 0
        shell_printf(ctrl, "%s", "\r\n mqtt <cmd> <cmd parameters> \
                                  \r\n  cmd: \
                                  \r\n    con - connect to MQTT broker \
                                  \r\n    con_ssl - secure connect to MQTT broker \
                                  \r\n    pub - publish topic message \
                                  \r\n    sub - subscribe topic \
                                  \r\n    unsub - unsubscribe topic \
                                  \r\n    ping - send a ping request to broker \
                                  \r\n    cls - close connection to MQTT broker ");
#endif
    }

    return ercd;
}

#endif  /* #if SAMPLE_ENA_MQTTc */
