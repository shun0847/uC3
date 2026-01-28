
#ifndef SAMPLE_ENA_HTTPd
#define SAMPLE_ENA_HTTPd        0
#endif
#ifndef SAMPLE_ENA_FTPd
#define SAMPLE_ENA_FTPd         0
#endif

#ifndef SAMPLE_ENA_FTPc
#define SAMPLE_ENA_FTPc         0
#endif
#ifndef SAMPLE_ENA_TFTPc
#define SAMPLE_ENA_TFTPc        0
#endif
#ifndef SAMPLE_ENA_HTTPc
#define SAMPLE_ENA_HTTPc        0
#endif
#ifndef SAMPLE_ENA_SMTPua
#define SAMPLE_ENA_SMTPua       0
#endif
#ifndef SAMPLE_ENA_SNTP
#define SAMPLE_ENA_SNTP         0
#endif
#ifndef SAMPLE_ENA_POP3c
#define SAMPLE_ENA_POP3c        0
#endif
#ifndef SAMPLE_ENA_TELNETd
#define SAMPLE_ENA_TELNETd      0
#endif
#ifndef SAMPLE_ENA_DHCPd
#define SAMPLE_ENA_DHCPd      0
#endif

#ifndef SAMPLE_ENA_MQTTc
#define SAMPLE_ENA_MQTTc        0
#endif

#ifndef SAMPLE_ENA_WEBSOCKETd
#define SAMPLE_ENA_WEBSOCKETd   0
#endif

#ifndef SAMPLE_USE_FFSYS
#define SAMPLE_USE_FFSYS        (SAMPLE_ENA_FTPd | SAMPLE_ENA_FTPc | SAMPLE_ENA_TFTPc | SAMPLE_ENA_SMTPua | SAMPLE_ENA_POP3c)
#endif

#ifndef SAMPLE_USE_GENSRC
#ifdef NET_C
#define SAMPLE_USE_GENSRC       1
#else
#define SAMPLE_USE_GENSRC       0
#endif
#endif

#ifndef SAMPLE_SOCDEV_CLI
#define SAMPLE_SOCDEV_CLI       0       /* Device number used for client socket */
#endif

#ifndef SAMPLE_SOCDEV_SER
#define SAMPLE_SOCDEV_SER       0       /* Device number used for server socket */
#endif

#if (_kernel_SIZE_SIZE==8)
#define EXEC_CPU_64BIT      1
#else
#define EXEC_CPU_64BIT      0
#endif
