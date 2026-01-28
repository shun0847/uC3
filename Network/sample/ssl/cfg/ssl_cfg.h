/***********************************************************************
  MICRO C CUBE / COMPACT/STANDARD, SSL
  User configuration

    Generated at 2015-10-27 20:10:46
 ***********************************************************************/

#ifndef _SSL_CFG_H_
#define _SSL_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
  * Protocol version
  * SSL_MINOR_V0 (SSL v3.0)
  * SSL_MINOR_V1 (TLS v1.0)
  * SSL_MINOR_V2 (TLS v1.1)
  * SSL_MINOR_V3 (TLS v1.2)
  */
#define CFG_SSL_VERSION         SSL_MINOR_V3
#define CFG_SSL_VERSION_MIN     SSL_MINOR_V1

#define CFG_SSL_CERT_SIZE       2048     /* Max certification size */
#define CFG_SSL_CERT_DEPTH      2     /* Max certification depth */
#define CFG_SSL_MAX_SESSION     3     /* Max number of sessions */
#define CFG_SSL_MAX_CONNECTION  1     /* Max number of connections */
#define CFG_SSL_CA_CERT_MAX     1     /* Max number of certificate authority */
#define CFG_SSL_CRYP_TMO        100     /* Timeout of cryptographic processor */
#define CFG_SSL_HASH_TMO        100     /* Timeout of HASH processor */
#define CFG_SSL_KEY_SIZE        2048     /* Max private key size */

#define SSL_NET_BUF_MPF_SZ      1576     /* Memory block size (ID_SSL_MPF) */

#ifdef __cplusplus
}
#endif
#endif
