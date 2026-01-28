/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    MQTT Client configuration

    Generated at 2017-10-03 11:28:54
                 2021-08-19 Update the MQTT_CBK_STKSZ for 64bit processor
 **************************************************************************/
#include "kernel.h"
#include "mqtt.h"

#ifdef NET_C_OS
#include "kernel_id.h"
#include "net_id.h"

ER mqtt_res_cre()
{
    return E_OK;
}
#else

#define MQTT_USE_DEV        1
#define MQTT_CLI_NUM        2
#define MQTT_MAX_MSGSIZE    256

#if (_kernel_SIZE_SIZE==8)  /* 64bit CPU */
#define MQTT_SND_STKSZ      (1024*3)
#define MQTT_RCV_STKSZ      (1024*3)
#define MQTT_CBK_STKSZ      (512*3)
#else
#define MQTT_SND_STKSZ      1024
#define MQTT_RCV_STKSZ      1024
#define MQTT_CBK_STKSZ       512
#endif

SID ID_MQTTC_SOC[MQTT_CLI_NUM];
ID ID_MQTTC_SND_TSK[MQTT_CLI_NUM];
ID ID_MQTTC_RCV_TSK[MQTT_CLI_NUM];
ID ID_MQTTC_APPRCV_TSK[MQTT_CLI_NUM];
ID ID_MQTTC_SND_MPF[MQTT_CLI_NUM];
ID ID_MQTTC_RCV_MPF[MQTT_CLI_NUM];
ID ID_MQTTC_LOC_SEM[MQTT_CLI_NUM];
ID ID_MQTTC_FLG[MQTT_CLI_NUM];

extern void mqtt_snd_tsk(VP_INT exinf);
extern void mqtt_rcv_tsk(VP_INT exinf);
extern void mqtt_apprcv_tsk(VP_INT exinf);

const T_CTSK c_mqtt_snd_tsk  = { TA_HLNG | TA_FPU, NULL, (FP)mqtt_snd_tsk,  5,  MQTT_SND_STKSZ, 0 };
const T_CTSK c_mqtt_rcv_tsk  = { TA_HLNG | TA_FPU, NULL, (FP)mqtt_rcv_tsk,  5,  MQTT_RCV_STKSZ, 0 };
const T_CTSK c_mqtt_apprcv_tsk  = { TA_HLNG | TA_FPU, NULL, (FP)mqtt_apprcv_tsk,  5,  MQTT_CBK_STKSZ, 0 };
const T_CMPF c_mqtt_snd_mpf  = { TA_TFIFO, 8, sizeof(T_MQTT_MSG) + MQTT_MAX_MSGSIZE };
const T_CMPF c_mqtt_rcv_mpf  = { TA_TFIFO, 8, sizeof(T_MQTT_MSG) + MQTT_MAX_MSGSIZE };
const T_CSEM c_mqtt_loc_sem  = { TA_TFIFO, 1, 1 };
const T_CFLG c_mqtt_flg  = { TA_TFIFO | TA_WMUL | TA_CLR, 0 };

ER mqtt_res_cre()
{
    ER ercd;
    T_NODE lo_host;
    UB n;

    lo_host.num = MQTT_USE_DEV;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;
    lo_host.port = PORT_ANY;

    for (n = 0; n < MQTT_CLI_NUM; n++) {
        /* MQTTc - socket */
        ercd = cre_soc(IP_PROTO_TCP, &lo_host);
        if (0 >= ercd)  break;
        ID_MQTTC_SOC[n] = ercd;
        cfg_soc(ercd, SOC_TMO_SND, (VP)10000);
        cfg_soc(ercd, SOC_TMO_RCV, (VP)-1);
        cfg_soc(ercd, SOC_TMO_CON, (VP)10000);
        cfg_soc(ercd, SOC_TMO_CLS, (VP)10000);

        /* MQTTc - kernel resource */
        ercd = acre_tsk((T_CTSK *)&c_mqtt_snd_tsk);
        if (0 >= ercd)  break;
        ID_MQTTC_SND_TSK[n] = ercd;

        ercd = acre_tsk((T_CTSK *)&c_mqtt_rcv_tsk);
        if (0 >= ercd)  break;
        ID_MQTTC_RCV_TSK[n] = ercd;

        ercd = acre_tsk((T_CTSK *)&c_mqtt_apprcv_tsk);
        if (0 >= ercd)  break;
        ID_MQTTC_APPRCV_TSK[n] = ercd;

        ercd = acre_mpf((T_CMPF *)&c_mqtt_snd_mpf);
        if (0 >= ercd)  break;
        ID_MQTTC_SND_MPF[n] = ercd;

        ercd = acre_mpf((T_CMPF *)&c_mqtt_rcv_mpf);
        if (0 >= ercd)  break;
        ID_MQTTC_RCV_MPF[n] = ercd;

        ercd = acre_sem((T_CSEM *)&c_mqtt_loc_sem);
        if (0 >= ercd)  break;
        ID_MQTTC_LOC_SEM[n] = ercd;

        ercd = acre_flg((T_CFLG *)&c_mqtt_flg);
        if (0 >= ercd)  break;
        ID_MQTTC_FLG[n] = ercd;
    }

    ercd = (0 <= ercd) ? E_OK : ercd ;

    return ercd;
}
#endif

T_MQTT_CLIENT   gMQTTClient[MQTT_CLI_NUM];
const char mqtt_client_id[] = "MQTTClient1234567837F4";
extern BOOL mqtt_rcv_callback(T_MQTT_CLIENT *p_cli, T_MQTT_PUB *p_pub);

ER mqtt_usr_ini(void)
{
    T_MQTT_CLIENT *p_cli;
    ER ercd;
    UB n;

    ercd = mqtt_res_cre();
    if (E_OK != ercd) {
        return ercd;
    }

    /* Initialize MQTT Client */
    for (n = 0; n < MQTT_CLI_NUM; n++) {
        p_cli = &gMQTTClient[n];
        p_cli->soc_id    = ID_MQTTC_SOC[n];
        p_cli->loc_semid = ID_MQTTC_LOC_SEM[n];
        p_cli->evt_flgid = ID_MQTTC_FLG[n];
        p_cli->tx_mpfid  = ID_MQTTC_SND_MPF[n];
        p_cli->rx_mpfid  = ID_MQTTC_RCV_MPF[n];
        p_cli->tx_tskid  = ID_MQTTC_SND_TSK[n];
        p_cli->rx_tskid  = ID_MQTTC_RCV_TSK[n];
        p_cli->rx_app_tskid  = ID_MQTTC_APPRCV_TSK[n];
        p_cli->cbk       = mqtt_rcv_callback;
        p_cli->max_msg_size = MQTT_MAX_MSGSIZE;

        ercd = mqtt_cli_cre(p_cli);
        if (ercd != E_OK) {
            return ercd;
        }
    }

    return  E_OK;
}

