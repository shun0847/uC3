/***********************************************************************
    MICRO C CUBE / COMPACT
    Double Linked list implementation
    Copyright (c) 2016-2021, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.08.01: Created.
      2021.08.07: In rmv_mqtt_lst(), when top or bottom element is removed
                  the nxt or prv pointer is not set correctly. Due to this
                  when ref_mqtt_lst_nxt() or ref_mqtt_lst_prv() may return
                  element which is already removed from queue.
************************************************************************/
#include "kernel.h"
#include "mqtt_util.h"
//#include "kernel_id.h"
#ifndef NULL
#ifdef __cplusplus
#define NULL    (0)
#else
#define NULL    ((void *)0)
#endif
#endif

ER ini_mqtt_lst(T_MQTT_LST *lst)
{
    lst->top = lst->btm = NULL;

    return E_OK;
}

ER add_mqtt_lst(T_MQTT_LST *lst, T_MQTT_LST_ELE *ele, BOOL top)
{
    ER ercd;
    
    ercd = E_PAR;

    if (lst && ele) {
        if (lst->top == NULL && lst->btm == NULL) {
            /* list is empty */
            lst->top = lst->btm = ele;
            ele->prv = ele;
            ele->nxt = NULL;
        }
        else {
            /* list is not empty, has one or more elements */
            if (top) {
                /* Add new element to the top of the list */
                ele->nxt = lst->top;
                lst->top->prv = ele;
                lst->top = ele;
                ele->prv = ele;
            }
            else {
                /* Add new element to the bottom of the list */
                ele->nxt = NULL;
                ele->prv = lst->btm;
                lst->btm->nxt = ele;
                lst->btm = ele;
            }    
        }

        ercd = E_OK;
    }

    return ercd;
}

T_MQTT_LST_ELE* ref_mqtt_lst(T_MQTT_LST *lst, BOOL top)
{
    T_MQTT_LST_ELE *ele;
    
    if (lst == NULL) {
        return NULL;
    }

    if (top) {
        ele = lst->top;
    }
    else {
        ele = lst->btm;
    }

    return ele;
}

T_MQTT_LST_ELE* ref_mqtt_lst_nxt(T_MQTT_LST_ELE *ele)
{
    T_MQTT_LST_ELE *nxt_ele = NULL;
    
    if (ele) {
        nxt_ele = ele->nxt;
    }
    
    return nxt_ele;
}

T_MQTT_LST_ELE* ref_mqtt_lst_prv(T_MQTT_LST_ELE *ele)
{
    T_MQTT_LST_ELE *prv_ele;
    
    if (ele == NULL) {
        return NULL;
    }

    prv_ele = ele->prv;
    if (prv_ele == ele) { /* points to same element */
        prv_ele = NULL; /* no previous element as 'ele' is the top element */
    }    
    
    return prv_ele;
}

void rmv_mqtt_lst(T_MQTT_LST *lst, T_MQTT_LST_ELE *ele)
{
    T_MQTT_LST_ELE *n;
    
    if ((lst == NULL) || (ele == NULL)) {
        return;
    }

    if (lst->top == ele && lst->btm == ele) {
        lst->top = NULL;
        lst->btm = NULL;
        return;
    }

    if (lst->top == ele) {
        lst->top = ele->nxt;
        n = ele->nxt;
        n->prv = n; /*set top element's prv to itself*/
    }    
    else if (lst->btm == ele) {
        lst->btm = ele->prv;
        n = ele->prv;
        n->nxt = NULL; /*last element's nxt should be NULL*/
    }
    else {
        n = ele->prv;
        n->nxt = ele->nxt;
        n = ele->nxt;
        n->prv = ele->prv; 
    }
    ele->nxt = ele->prv = NULL;
}

BOOL mqtt_chk_utf8(UB *c, UH len, UH *code_bytes)
{
    *code_bytes = 0;
    if ((c == NULL) || (len == 0)) {
        return FALSE;
    }

    if ((c[0] & 0xF8) == 0xF0) {
        /* 4 byte */
        if (len < 4) {
            return FALSE;
        }
        if (((c[1] & 0xC0) != 0x80) ||
            ((c[2] & 0xC0) != 0x80) ||
            ((c[3] & 0xC0) != 0x80)) {
            return FALSE;
        }
        *code_bytes = 4;
    }
    else if ((c[0] & 0xF0) == 0xE0) {
        /* 3 byte */
        if (len < 3) {
            return FALSE;
        }
        if (((c[1] & 0xC0) != 0x80) ||
            ((c[2] & 0xC0) != 0x80)) {
            return FALSE;
        }
        *code_bytes = 3;
    }
    else if ((c[0] & 0xE0) == 0xC0) {
        /* 2 byte */
        if (len < 2) {
            return FALSE;
        }
        if ((c[1] & 0xC0) != 0x80) {
            return FALSE;
        }
        *code_bytes = 2;
    }
    else {
        /* 0x00 - 0x7F */
        //if (!((c[0] >= 0) && (c[0] <= 0x7F))) {
        if (c[0] > 0x7F) {
            return FALSE;
        }
        *code_bytes = 1;
    }

    return TRUE;
}

static UW mqtt_utf8_bad_code4[] = {
    0xF09FBFBE, 0xF09FBFBF, 0xF0AFBFBE, 0xF0AFBFBF, 0xF0BFBFBE, 0xF0BFBFBF,
    0xF18FBFBE, 0xF18FBFBF, 0xF19FBFBE, 0xF19FBFBF, 0xF1AFBFBE, 0xF1AFBFBF, 0xF1BFBFBE, 0xF1BFBFBF,
    0xF28FBFBE, 0xF28FBFBF, 0xF29FBFBE, 0xF29FBFBF, 0xF2AFBFBE, 0xF2AFBFBF, 0xF2BFBFBE, 0xF2BFBFBF,
    0xF38FBFBE, 0xF38FBFBF, 0xF39FBFBE, 0xF39FBFBF, 0xF3AFBFBE, 0xF3AFBFBF, 0xF3BFBFBE, 0xF3BFBFBF,
    0xF48FBFBE, 0xF48FBFBF
};

BOOL mqtt_chk_utf8_codepoint(UB *c, UH len, UB chk_topic)
{
    UW code;
    UH b, i;
    UB *p = NULL;

    if ((c == NULL) || (len == 0)) {
        return FALSE;
    }

    while (len) {
        if (!mqtt_chk_utf8(c, len, &b)) {
            return FALSE;
        }

        switch (b) {
            case 1:
                /* 0, 1 to 1F, 7F not allowed */
                if (!((c[0] > 0x1F) && (c[0] < 0x7F))) {
                    return FALSE;
                }
                break;
            case 2:
                code = c[0] << 8 | c[1];
                if (!(code >= 0xC2A0 && code <= 0xDFBF)) {
                    return FALSE;
                }
                break;
            case 3:
                code = c[0] << 16 | c[1] << 8 | c[2];
                if (!(code >= 0xE0A080 && code <= 0xEFBFBF)) {
                    return FALSE;
                }
                if ((code >= 0xEDA080 && code <= 0xEDBFBF) ||
                    (code >= 0xEFB790 && code <= 0xEFB7AF) ||
                    (code >= 0xEFBFBE && code <= 0xEFBFBF)) {
                    return FALSE;
                }
                break;
            case 4:
                code = c[0] << 24 | c[1] << 16 | c[2] << 8 | c[3];
                code = code & 0xFFFFFFFF; /* unit test fail if not */
                if (!(code >= 0xF0908080 && code <= 0xF48FBFBF)) {
                    return FALSE;
                }
                for (i = 0; i < sizeof(mqtt_utf8_bad_code4)/4; i++) {
                    if (code == mqtt_utf8_bad_code4[i]) {
                        return FALSE;
                    }
                }
                break;
            default:
                return FALSE;
                //break;
        }

        if (chk_topic && b == 1) {

            if (chk_topic == 1) {
                /* Topic name cannot have wild cards + or # */
                if (c[0] == '+' || c[0] == '#') {
                    return FALSE;
                }
            }
            else if (chk_topic == 2) {
                /* validate topic filter */
                if (c[0] == '+') {
                    /* if a character present before or after + it should be / */
                    if ((p != NULL) && (p[0] != '/')) {
                        return FALSE;
                    }
                    if (len > 1 && c[1] != '/') {
                        return FALSE;
                    }
                }
                if (c[0] == '#') {
                    /* after # there should be no character */
                    if (len > 1) {
                        return FALSE;
                    }
                    /* if a character present before # it should be / */
                    if ((p != NULL) && (p[0] != '/')) {
                        return FALSE;
                    }
                }
            }

            p = c;  /* store previous char pointer */
        }
        else {
            p = NULL; /* not 1 byte code point, reset previous char pointer */
        }

        c += b;
        len -= b;
    }

    return TRUE;
}

/* Client Identifier should have only alpha numeric characters */
BOOL mqtt_chk_client_id(UB *str, UH len)
{
    if ((str == NULL) || (len == 0)) {
        return FALSE;
    }

    if (len > 23) {
        return FALSE;
    }

    while (len) {
        if (!((*str >= '0' && *str <= '9') ||
              (*str >= 'A' && *str <= 'Z') ||
              (*str >= 'a' && *str <= 'z'))) {
            return FALSE;
        }
        str++;
        len--;
    }
    return TRUE;
}

BOOL mqtt_chk_user_name(UB *str, UH len)
{
    return mqtt_chk_utf8_codepoint(str, len, 0);
}

BOOL mqtt_chk_pub_topic(UB *str, UH len)
{
    return mqtt_chk_utf8_codepoint(str, len, 1);
}

BOOL mqtt_chk_sub_topic(UB *str, UH len)
{
    return mqtt_chk_utf8_codepoint(str, len, 2);
}
