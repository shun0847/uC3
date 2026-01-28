/***********************************************************************
    MICRO C CUBE / COMPACT
    Double Linked list declarations
    Copyright (c) 2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.08.01: Created.
************************************************************************/

#ifndef __MQTT_UTIL_H__
#define __MQTT_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct t_mqtt_lst_ele
{
    struct t_mqtt_lst_ele *nxt;
    struct t_mqtt_lst_ele *prv;
}T_MQTT_LST_ELE;

typedef struct t_mqtt_lst
{
    T_MQTT_LST_ELE *top;
    T_MQTT_LST_ELE *btm;
}T_MQTT_LST;

#define LST_TOP     TRUE
#define LST_BOTTOM  FALSE

ER ini_mqtt_lst(T_MQTT_LST *lst);
ER add_mqtt_lst(T_MQTT_LST *lst, T_MQTT_LST_ELE *ele, BOOL top);
void rmv_mqtt_lst(T_MQTT_LST *lst, T_MQTT_LST_ELE *ele);
T_MQTT_LST_ELE* ref_mqtt_lst(T_MQTT_LST *lst, BOOL top);
T_MQTT_LST_ELE* ref_mqtt_lst_nxt(T_MQTT_LST_ELE *ele);
T_MQTT_LST_ELE* ref_mqtt_lst_prv(T_MQTT_LST_ELE *ele);

BOOL mqtt_chk_client_id(UB *str, UH len);
BOOL mqtt_chk_user_name(UB *str, UH len);
BOOL mqtt_chk_pub_topic(UB *str, UH len);
BOOL mqtt_chk_sub_topic(UB *str, UH len);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __MQTT_UTIL_H__ */
