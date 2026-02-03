/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    String Library
    Copyright (c) 2012-2021, eForce Co., Ltd. All rights reserved.

    Version Information
      2012.10.02: Created
      2013.01.28: Fixed to be terminated NULL str1 in net_strcpy(), net_strcat()
      2013.06.28: Add net_strstr(), net_strcasestr()
      2014.03.31: Add net_strncmp(), net_strncat(), net_atoi(), net_itoa()
                  , net_ltoa()
      2014.04.01: Changed to ANSI equivalent to the type of arguments and re-
                  turn value. But, 'size_t' was replace by 'SIZE' in uITRON.
      2014.07.10: Suppressed warning of the GCC compiler.
      2015.03.31: Add net_strncpy()
      2015.04.13: Fixed the end string comparison in net_strncmp()
      2016.05.19: Add net_utoa()
      2016.08.04: Fixed a problem that does not become a negative value in net_itoa.
      2017.03.07: Add net_strcasecmp()
      2017.07.27: Support 64bit processor
      2021.10.14: Add net_strnlen()
 ***************************************************************************/

#include "net_strlib.h"

#ifndef NULL
#define NULL    ((void*)0)
#endif

int net_atoi(const char *str)
{
    int ret, mul;

    mul = ('-' == *str)  ? -1 : 1 ;
    if (0 > mul)    ++str;

    for (ret = 0; (('0' <= *str) && (*str <= '9')); ++str) {
        ret = ret * 10 + (*str - '0');
    }

    return (ret * mul);
}

long net_atol(const char *str)
{
    long ret, mul;

    mul = ('-' == *str)  ? -1 : 1 ;
    if (0 > mul)    ++str;

    for (ret = 0; (('0' <= *str) && (*str <= '9')); ++str) {
        ret = ret * 10 + (*str - '0');
    }

    return (ret * mul);
}


char* net_utoa(unsigned int num, char *str, int base)
{
    unsigned int tmp;
    unsigned int unum;
    unsigned int len;
    char c;
    char *tbuf;

    if (2 > base)   return NULL;
    tbuf = str;
    unum = (unsigned int)num;

    if (unum) {
        len = 0;
        for (tmp = unum; tmp; tmp /= ((unsigned int)base)) {
            ++len;
        }
    }
    else {
        len = 1;
    }
    tbuf[len] = '\0';

    for (tmp = num; len; --len) {
        c = (tmp % ((unsigned int)base));
        c += (10 <= c) ? ('A' - 10) : ('0');
        tbuf[len - 1] = c;
        tmp = tmp / ((unsigned int)base);
    }

    return str;
}

char* net_itoa(int num, char *str, int base)
{
    char *tbuf;

    if (2 > base)   return NULL;
    if ((10 == base) && (0 > num)) {
        num = -num;
        str[0] = '-';
        tbuf = &str[1];
    }
    else {
        tbuf = str;
    }
    net_utoa((unsigned int)num, tbuf, base);
    return str;
}


int net_strncmp(const char *str1, const char *str2, SIZE len)
{
    SIZE c;
    
    for (c = 0; (len && (*str1 || *str2)); --len, ++str1, ++str2) {
        if (0 != (c = *str1 - *str2)) {
            break;
        }
    }

    return (len) ? ((int)c) : 0 ;
}

int net_strcmp(const char *str1, const char *str2)
{
    int c=0;
    while ((*str1) || (*str2)) {
        c = *str1 - *str2;
        if (c) {
            break;
        }
        str1++;
        str2++;
    }
    return c;
}

char* net_strcpy(char *str1, const char *str2)
{
    char *t = str1;
    do {
        *str1 = *str2;
        str1++;
    }while(*str2++);
    return t;
}

SIZE net_strnlen(const char *str, SIZE len)
{
    SIZE l=0;
    while (len && *str) {
        str++;
        l++;
        len--;
    }
    return l;
}

SIZE net_strlen(const char *str)
{
    SIZE l=0;
    while (*str) {
        str++;
        l++;
    }
    return l;
}


char* net_strncat(char *str1, const char *str2, SIZE len)
{
    char *t = str1;
    
    while (*str1) ++str1;
    
    for (; len && *str2; --len, ++str2, ++str1) {
        *str1 = *str2;
    }
    *str1 = '\0';
    
    return t;
}

char* net_strcat(char *str1, const char *str2)
{
    char *t = str1;
    while (*str1) str1++;
    net_strcpy(str1, str2);
    return t;
}

char* net_strchr(const char *str, int ch)
{
    while (*str) {
        if (*str == (char)ch) {
            return (char*)str;
        }
        str++;
    }
    if (*str == ch) {
        return (char*)str;
    }
    return (char*)0;
}

extern int net_memcmp(const void *d, const void *s, SIZE n);
char* net_strstr(const char *str1, const char *str2)
{
    const char *ret;
    SIZE len;
    
    len = net_strlen(str2);
    ret = str1;
    for (;;) {
        ret = net_strchr(ret, str2[0]);
        if (0 == ret)   break;
        
        if (net_memcmp((VP)ret, (VP)str2, len) == 0) {
            break;
        }
        else {
            ++ret;
        }
    }
    
    return (char *)ret;
}

#define IS_UPPER(c)   ('A' <= (c) && (c) <= 'Z')
#define TO_LOWER(c)   (IS_UPPER(c) ? ((c) + 0x20) : (c))
int net_strncasecmp(const char *str1, const char *str2, SIZE len)
{
    int c1=0, c2=0;
    while ((len > 0) && ((*str1) || (*str2))) {
        c1 = TO_LOWER(*str1);
        c2 = TO_LOWER(*str2);
        if (c1 != c2) {
            break;
        }
        len--;
        str1++;
        str2++;
    }
    return (c1-c2);
}

int net_strcasecmp(const char *str1, const char *str2)
{
    int c1=0, c2=0;
    while (((*str1) || (*str2))) {
        c1 = TO_LOWER(*str1);
        c2 = TO_LOWER(*str2);
        if (c1 != c2) {
            break;
        }
        str1++;
        str2++;
    }
    return (c1-c2);
}

char *net_strcasestr(const char *str1, const char *str2)
{
    SIZE n1, n2;
    const char *ret;
    
    if ('\0' == str2[0]) return (char*)0;
    
    n1 = n2 = 0;
    ret = 0;
    while (str1[n1]) {
        if (TO_LOWER(str1[n1 + n2]) == TO_LOWER(str2[n2])) {
            ++n2;
            
            if ('\0' == str2[n2]) {
                ret = (char *)&str1[n1];
                break;
            }
            else if ('\0' == str1[n1 + n2]) {
                break;
            }
        }
        else {
            ++n1;
            n2 = 0;
        }
                                           
    }
    return (char *)ret;
}

char* net_strncpy(char *str1, const char *str2, SIZE len)
{
    SIZE i;

    for (i=0 ; i<len && str2[i]!='\0' ; i++) {
        str1[i] = str2[i];
    }
    for ( ; i<len ; i++) {
        str1[i] = '\0';
    }

    return str1;
}
