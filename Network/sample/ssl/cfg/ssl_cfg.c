/***********************************************************************
  MICRO C CUBE / COMPACT/STANDARD, SSL
  User configuration
 ***********************************************************************/

#include <string.h>
#include "kernel.h"
#include "net_hdr.h"
#include "ssl_cfg.h"
#include "ssl_hdr.h"
#include "ssl_utl.h"
#include "ssl_def.h"
#include "ssl_cph.h"
#include "ssl_arc4.h"

/* Cipher suites */
T_SSL_CIPHER_SPEC const gSSL_CFG_CIPHER_SPEC_LIST[] = {
    {TLS_RSA_WITH_RC4_128_MD5, arc4_ini, arc4_enc, 16, 0},         /* RC4 */
    {TLS_RSA_WITH_RC4_128_SHA, arc4_ini, arc4_enc, 16, 0},         /* RC4 */
//    {TLS_RSA_WITH_3DES_EDE_CBC_SHA, NULL, tdes_enc, 24, 8},        /* TDES */     /* not support */
    {TLS_RSA_WITH_AES_128_CBC_SHA, NULL, aes128_enc, 16, 16},      /* AES128 */
    {TLS_RSA_WITH_AES_256_CBC_SHA, NULL, aes256_enc, 32, 16},      /* AES256 */
    {TLS_RSA_WITH_AES_128_CBC_SHA256, NULL, aes128_enc, 16, 16},   /*AES128 */
    {TLS_RSA_WITH_AES_256_CBC_SHA256, NULL, aes256_enc, 32, 16},   /*AES256 */
    {TLS_NULL_WITH_NULL_NULL, NULL, NULL, 0, 0}
};

/* User configuration values */
T_SSL_CFG_USR_VAL const gSSL_CFG_USR_VAL = {
    CFG_SSL_VERSION,
    CFG_SSL_CERT_SIZE,
    CFG_SSL_CERT_DEPTH,
    CFG_SSL_MAX_SESSION,
    CFG_SSL_MAX_CONNECTION,
    CFG_SSL_CA_CERT_MAX,
    CFG_SSL_CRYP_TMO,
    CFG_SSL_HASH_TMO,
    SSL_NET_BUF_MPF_SZ,
    CFG_SSL_KEY_SIZE,
    CFG_SSL_VERSION_MIN
};

/* OS configuration values */
T_SSL_CFG_OS_VAL gSSL_CFG_OS_VAL = {0};
const T_CMPF c_ssl_mpf = {TA_TFIFO, 12, SSL_NET_BUF_MPF_SZ, NULL, NULL};
const T_CMPF c_hash_mpf = {TA_TFIFO, 6, 64, 0};


/* Global variables */
T_SSL_CON gSSL_CON[CFG_SSL_MAX_CONNECTION];
T_SSL_SSN gSSL_SSN[CFG_SSL_MAX_SESSION];
T_CERT_X509 gSSL_CA_CERT[CFG_SSL_CA_CERT_MAX];
T_CERT_X509 gSSL_SERVER_CERT[(CFG_SSL_CERT_DEPTH) * (CFG_SSL_MAX_SESSION)];
UW gSSL_CA_CERT_BUF[(((CFG_SSL_CA_CERT_MAX) * (CFG_SSL_CERT_SIZE)) + sizeof(UW)) / sizeof(UW)];
UW gSSL_CERT_BUF[((((CFG_SSL_CERT_DEPTH) * (CFG_SSL_MAX_SESSION)) * (CFG_SSL_CERT_SIZE)) + sizeof(UW)) / sizeof(UW)];

/* client certification */
T_CERT_X509 gSSL_CLIENT_CERT;
UW gSSL_CLIENT_CERT_BUF[(CFG_SSL_CERT_SIZE + sizeof(UW)) / sizeof(UW)];
UW gSSL_CLIENT_KEY_BUF[(CFG_SSL_KEY_SIZE + sizeof(UW)) / sizeof(UW)];
