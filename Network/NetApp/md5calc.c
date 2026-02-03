/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    MD5 Message-Digest Algorithm Implementation
    Copyright (c)  2013, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.06.24: Created
      2013.09.27: Add HMAC-MD5 processing function
 ***************************************************************************/

#include "kernel.h"
#include "md5calc.h"
#include "net_hdr.h"

/* ver1.xx */
#if 1
#include "string.h"
#define net_memcpy      memcpy
#define net_memcmp      memcmp
#define net_memset      memset
#define net_strlen      strlen
#define net_strchr      strchr
#define net_strcat      strcat
#define net_strncasecmp strncasecmp
#else
#include "net_strlib.h"
#endif

static const UB md5_pad[64] = {
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z))) 

#define RL(x, n) (((x) << (n)) | ((x) >> (32-(n))))

#define TR1(a, b, c, d, x, s, i) \
  {(a) += F((b),(c),(d)) + (x) + (i); \
   (a) = RL((a), (s)); \
   (a) += (b); \
  }
#define TR2(a, b, c, d, x, s, i) \
  {(a) += G ((b), (c), (d)) + (x) + (UW)(i); \
   (a) = RL((a), (s)); \
   (a) += (b); \
  }
#define TR3(a, b, c, d, x, s, i) \
  {(a) += H ((b), (c), (d)) + (x) + (UW)(i); \
   (a) = RL((a), (s)); \
   (a) += (b); \
  }
#define TR4(a, b, c, d, x, s, i) \
  {(a) += I ((b), (c), (d)) + (x) + (UW)(i); \
   (a) = RL((a), (s)); \
   (a) += (b); \
  }



void md5i_init(MD5_INFO *md5i)
{
    md5i->len = 0;
    
    md5i->var[0] = 0x67452301;
    md5i->var[1] = 0xefcdab89;
    md5i->var[2] = 0x98badcfe;
    md5i->var[3] = 0x10325476;
}

void md5i_transform(MD5_INFO *md5i)
{
    UW a,b,c,d;
    UW msg[16], *n;
    UB *buf;

    
    /* 64char -> 16long */
    n = &msg[0];
    buf = &md5i->buf[0];
    for (a=0;a<16;a++) {
        *n = buf[3];
        *n = (*n << 8) + buf[2];
        *n = (*n << 8) + buf[1];
        *n = (*n << 8) + buf[0];
        buf += 4;
        n++;
    }

    a = md5i->var[0]; /* aa */
    b = md5i->var[1]; /* bb */
    c = md5i->var[2]; /* cc */
    d = md5i->var[3]; /* dd */

    /* Round 1
        [abcd k s i]
        a = b + ((a + F(b,c,d) + X[k] + T[i]) <<< s)
    */
    TR1(a, b, c, d, msg[ 0],  7, 0xd76aa478);
    TR1(d, a, b, c, msg[ 1], 12, 0xe8c7b756);
    TR1(c, d, a, b, msg[ 2], 17, 0x242070db);
    TR1(b, c, d, a, msg[ 3], 22, 0xc1bdceee);
    TR1(a, b, c, d, msg[ 4],  7, 0xf57c0faf);
    TR1(d, a, b, c, msg[ 5], 12, 0x4787c62a);
    TR1(c, d, a, b, msg[ 6], 17, 0xa8304613);
    TR1(b, c, d, a, msg[ 7], 22, 0xfd469501);
    TR1(a, b, c, d, msg[ 8],  7, 0x698098d8);
    TR1(d, a, b, c, msg[ 9], 12, 0x8b44f7af);
    TR1(c, d, a, b, msg[10], 17, 0xffff5bb1);
    TR1(b, c, d, a, msg[11], 22, 0x895cd7be);
    TR1(a, b, c, d, msg[12],  7, 0x6b901122);
    TR1(d, a, b, c, msg[13], 12, 0xfd987193);
    TR1(c, d, a, b, msg[14], 17, 0xa679438e);
    TR1(b, c, d, a, msg[15], 22, 0x49b40821);

    /* Round 2
        [abcd k s i]
        a = b + ((a + G(b,c,d) + X[k] + T[i]) <<< s)
    */
    TR2(a, b, c, d, msg[ 1],  5, 0xf61e2562);
    TR2(d, a, b, c, msg[ 6],  9, 0xc040b340);
    TR2(c, d, a, b, msg[11], 14, 0x265e5a51);
    TR2(b, c, d, a, msg[ 0], 20, 0xe9b6c7aa);
    TR2(a, b, c, d, msg[ 5],  5, 0xd62f105d);
    TR2(d, a, b, c, msg[10],  9,  0x2441453);
    TR2(c, d, a, b, msg[15], 14, 0xd8a1e681);
    TR2(b, c, d, a, msg[ 4], 20, 0xe7d3fbc8);
    TR2(a, b, c, d, msg[ 9],  5, 0x21e1cde6);
    TR2(d, a, b, c, msg[14],  9, 0xc33707d6);
    TR2(c, d, a, b, msg[ 3], 14, 0xf4d50d87);
    TR2(b, c, d, a, msg[ 8], 20, 0x455a14ed);
    TR2(a, b, c, d, msg[13],  5, 0xa9e3e905);
    TR2(d, a, b, c, msg[ 2],  9, 0xfcefa3f8);
    TR2(c, d, a, b, msg[ 7], 14, 0x676f02d9);
    TR2(b, c, d, a, msg[12], 20, 0x8d2a4c8a);

    /* Round 3
        [abcd k s i] 
        a = b + ((a + H(b,c,d) + X[k] + T[i]) <<< s)
    */
    TR3(a, b, c, d, msg[ 5],  4, 0xfffa3942);
    TR3(d, a, b, c, msg[ 8], 11, 0x8771f681);
    TR3(c, d, a, b, msg[11], 16, 0x6d9d6122);
    TR3(b, c, d, a, msg[14], 23, 0xfde5380c);
    TR3(a, b, c, d, msg[ 1],  4, 0xa4beea44);
    TR3(d, a, b, c, msg[ 4], 11, 0x4bdecfa9);
    TR3(c, d, a, b, msg[ 7], 16, 0xf6bb4b60);
    TR3(b, c, d, a, msg[10], 23, 0xbebfbc70);
    TR3(a, b, c, d, msg[13],  4, 0x289b7ec6);
    TR3(d, a, b, c, msg[ 0], 11, 0xeaa127fa);
    TR3(c, d, a, b, msg[ 3], 16, 0xd4ef3085);
    TR3(b, c, d, a, msg[ 6], 23,  0x4881d05);
    TR3(a, b, c, d, msg[ 9],  4, 0xd9d4d039);
    TR3(d, a, b, c, msg[12], 11, 0xe6db99e5);
    TR3(c, d, a, b, msg[15], 16, 0x1fa27cf8);
    TR3(b, c, d, a, msg[ 2], 23, 0xc4ac5665);

    /* Round 4
        [abcd k s i] 
        a = b + ((a + I(b,c,d) + X[k] + T[i]) <<< s)
    */
    TR4(a, b, c, d, msg[ 0],  6, 0xf4292244);
    TR4(d, a, b, c, msg[ 7], 10, 0x432aff97);
    TR4(c, d, a, b, msg[14], 15, 0xab9423a7);
    TR4(b, c, d, a, msg[ 5], 21, 0xfc93a039);
    TR4(a, b, c, d, msg[12],  6, 0x655b59c3);
    TR4(d, a, b, c, msg[ 3], 10, 0x8f0ccc92);
    TR4(c, d, a, b, msg[10], 15, 0xffeff47d);
    TR4(b, c, d, a, msg[ 1], 21, 0x85845dd1);
    TR4(a, b, c, d, msg[ 8],  6, 0x6fa87e4f);
    TR4(d, a, b, c, msg[15], 10, 0xfe2ce6e0);
    TR4(c, d, a, b, msg[ 6], 15, 0xa3014314);
    TR4(b, c, d, a, msg[13], 21, 0x4e0811a1);
    TR4(a, b, c, d, msg[ 4],  6, 0xf7537e82);
    TR4(d, a, b, c, msg[11], 10, 0xbd3af235);
    TR4(c, d, a, b, msg[ 2], 15, 0x2ad7d2bb);
    TR4(b, c, d, a, msg[ 9], 21, 0xeb86d391);

    md5i->var[0] += a;
    md5i->var[1] += b;
    md5i->var[2] += c;
    md5i->var[3] += d;
}

void md5i_append(MD5_INFO *md5i, const UB *msg, UW msg_len)
{
    UW i;
    W pos;

    /* Update message to md5 buffer and transform */
    /* the content if length exceeds 64 bytes.    */
    pos = (md5i->len >> 3) & 0x3F;
    md5i->len += (msg_len << 3);

    /* Append to buffer */
    for (i=0; i<msg_len;i++) {
        md5i->buf[pos++] = *msg++;
        if (pos == 64) {
            pos = 0;
            md5i_transform(md5i);
        }
    }
}

void md5i_finish(MD5_INFO *md5i, HASH hash)
{
    UB *p;
    W i;
    UW pad, md_len = md5i->len;
    
    /* Pad length */
    i = (md5i->len >> 3) & 0x3F;
    pad = (i < 56) ? (56 - i) : (120 - i);
    md5i_append(md5i, md5_pad, pad);

    /* update actual message length */
    md5i->buf[56] = (UB)(md_len & 0xFF);
    md5i->buf[57] = (UB)(md_len >> 8) & 0xFF;
    md5i->buf[58] = (UB)(md_len >> 16) & 0xFF;
    md5i->buf[59] = (UB)(md_len >> 24) & 0xFF;

    md5i->buf[63] = 0;
    md5i->buf[62] = 0;
    md5i->buf[61] = 0;
    md5i->buf[60] = 0;

    md5i_transform(md5i);

    /* Output */
    p = (UB *)hash;
    for (i=0; i < 4; i++) {
        *p++ = (UB)(md5i->var[i] & 0xFF);
        *p++ = (UB)(md5i->var[i] >> 8) & 0xFF;
        *p++ = (UB)(md5i->var[i] >> 16) & 0xFF;
        *p++ = (UB)(md5i->var[i] >> 24) & 0xFF;
    }
}

void md5_hash(UB *bytes, UW len, HASH hash)
{
    MD5_INFO md5i;
    
    md5i_init(&md5i);
    md5i_append(&md5i, bytes, len);
    md5i_finish(&md5i, hash);
}


void cnv_hex(HASH bin, HASHHEX hex)
{
    UH ni;
    UB nj;
    
    for (ni = 0; ni < HASHLEN; ++ni) {
        nj = (bin[ni] >> 4) & 0xf;
        if (nj <= 9) {
            hex[ni * 2] = (nj + '0');
        }
        else {
            hex[ni * 2] = (nj + 'a' - 10);
        }
        
        nj = bin[ni] & 0xf;
        if (nj <= 9) {
            hex[ni * 2 + 1] = (nj + '0');
        }
        else {
            hex[ni * 2 + 1] = (nj + 'a' - 10);
        }
    }
    hex[HASHHEXLEN] = '\0';
}

void cnv_n_hex(char *bin, char *hex, UW len)
{
    UH ni;
    UB nj;
    
    for (ni = 0; ni < len; ++ni) {
        nj = (bin[ni] >> 4) & 0xf;
        if (nj <= 9) {
            hex[ni * 2] = (nj + '0');
        }
        else {
            hex[ni * 2] = (nj + 'a' - 10);
        }
        
        nj = bin[ni] & 0xf;
        if (nj <= 9) {
            hex[ni * 2 + 1] = (nj + '0');
        }
        else {
            hex[ni * 2 + 1] = (nj + 'a' - 10);
        }
    }
    hex[len << 1] = '\0';    
}

void hmac_md5(UB *txt, W txt_len, UB *k, W k_len, HASH *ha)
{
    MD5_INFO md5i;
    UB k_ipad[65], k_opad[65];
    int i;
    
    if (k_len > 64) {
        md5i_init(&md5i);
        md5i_append(&md5i, k, k_len);
        md5i_finish(&md5i, *ha);
        k = (unsigned char *)*ha;
        k_len = 16;
    }
    
    net_memset(k_ipad, 0, sizeof(k_ipad));
    net_memset(k_opad, 0, sizeof(k_opad));
    net_memcpy((VP)k_ipad, k, k_len);
    net_memcpy((VP)k_opad, k, k_len);
    
    for (i = 0; i < 64; ++i) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }
    
    /* perform inner MD5 */
    md5i_init(&md5i);
    md5i_append(&md5i, k_ipad, 64);
    md5i_append(&md5i, txt, txt_len);
    md5i_finish(&md5i, *ha);
    
    /* perform outer MD5 */
    md5i_init(&md5i);
    md5i_append(&md5i, k_opad, 64);
    md5i_append(&md5i, (UB *)*ha, 16);
    md5i_finish(&md5i, *ha);
}

