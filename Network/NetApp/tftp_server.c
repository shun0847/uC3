/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    TFTP Server
    Copyright (c)  2013-2021, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.07.12: Created
      2016.10.03: Execute static analysis tool to this source.
      2017.03.24: Improved the problem of tasks occupation.
	  2019.06.13: Fixed "Sorcerer's Apprentice Syndrome".
      2021.10.14: Added size check process for request file name.
                  Fixed sending an invalid packet when fopen failed.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_strlib.h"
#include "tftp_server.h"

#ifdef _UC3_ENDIAN_LITTLE
#define NTOHS(x)            ((((UH)(x) & 0xFFU) << 8U) | (((UH)(x) & 0xFF00U) >> 8U))
#else
#define NTOHS(x)            (x)
#endif

/** Definition for TFTP Packet **/
#define TFTP_OP_RRQ         (NTOHS(0x0001))
#define TFTP_OP_WRQ         (NTOHS(0x0002))
#define TFTP_OP_DATA        (NTOHS(0x0003))
#define TFTP_OP_ACK         (NTOHS(0x0004))
#define TFTP_OP_ERROR       (NTOHS(0x0005))

#define TFTP_MD_ASCII       "netascii"
#define TFTP_MD_BINARY      "octet"
/* "mail" is not support type  */

#define TFTP_ERC_NODEFINED  (NTOHS(0x0000))
#define TFTP_ERC_NOFILE     (NTOHS(0x0001))
#define TFTP_ERC_ACCESS     (NTOHS(0x0002))
#define TFTP_ERC_DISKFULL   (NTOHS(0x0003))
#define TFTP_ERC_OPERATE    (NTOHS(0x0004))
#define TFTP_ERC_UNKNOWN    (NTOHS(0x0005))
#define TFTP_ERC_EXISTFILE  (NTOHS(0x0006))
#define TFTP_ERC_NOUSER     (NTOHS(0x0007))

/** Definition for TFTP Error Messages **/
#define TFTP_ERS_NODEFINED  "Unknown error."
#define TFTP_ERS_NOFILE     "File not found."
#define TFTP_ERS_ACCESS     "Access violation."
#define TFTP_ERS_DISKFULL   "Disk full or allocation exceeded."
#define TFTP_ERS_OPERATE    "Illegal TFTP operation."
#define TFTP_ERS_UNKNOWN    "Unknown transfer ID."
#define TFTP_ERS_EXISTFILE  "File already exists."
#define TFTP_ERS_NOUSER     "No such user."
#define TFTP_MAX_ERSLEN     48

#define TFTP_BLKS_LEN       512U

#ifndef TFTP_TMO_NEXT_WAIT
#define TFTP_TMO_NEXT_WAIT  10U     /* next request wait when error */
#endif

#ifndef TFTP_FILENAME_MAX
#define TFTP_FILENAME_MAX   256U    /* Maximum length of filename in TFTP field */
#endif
#if (256 < TFTP_FILENAME_MAX)
  #error "Set a value of 256 or less for TFTP_FILENAME_MAX."
#endif


/** Structure definition **/
typedef struct t_err_code {
    UH cd;
    VB *msg;
} T_ERR_CODE;

typedef struct t_tftp_pkt {
    UH ope_cd;
    union {
        struct {
            VB buf[sizeof(UH) + TFTP_BLKS_LEN];
        } req;
        struct {
            UH blk_num;
            VB dat[TFTP_BLKS_LEN];
        } dat;
    } d;
} T_TFTP_PKT;

typedef struct t_tftp_pkt_err {
    UH ope_cd;
    struct {
        UH cd;
        VB buf[TFTP_MAX_ERSLEN];
    } err;
} T_TFTP_PKT_ERR;

enum TFTP_STATUS {
    TFS_SEND_ACK, TFS_RECV_ACK, TFS_RECV_DATA, TFS_SEND_DATA, 
    TFS_WRITE_FILE, TFS_READ_FILE, TFS_VALID_RECV, TFS_VALID_SEND, 
    TFS_SEND_REQ, TFS_UNKNOWN
};

/*----------------------------------------------------------------*/
/* Send TFTP error packet */
static ER tftp_snd_err(T_TFTP_SERVER *tftp, UH err_cd, VB *nodef_msg)
{
    const T_ERR_CODE el[] = {
        {(UH)TFTP_ERC_NOFILE,   TFTP_ERS_NOFILE},
        {(UH)TFTP_ERC_ACCESS,   TFTP_ERS_ACCESS},
        {(UH)TFTP_ERC_DISKFULL, TFTP_ERS_DISKFULL},
        {(UH)TFTP_ERC_OPERATE,  TFTP_ERS_OPERATE},
        {(UH)TFTP_ERC_UNKNOWN,  TFTP_ERS_UNKNOWN},
        {(UH)TFTP_ERC_EXISTFILE,TFTP_ERS_EXISTFILE},
        {(UH)TFTP_ERC_NOUSER,   TFTP_ERS_NOUSER},
    };
    T_TFTP_PKT_ERR pkte;
    UB ni;
    UH len;
    
    for (ni = 0U; ni < (UB)(sizeof(el)/sizeof(el[0])); ++ni) {
        if (el[ni].cd == err_cd) {
            break;
        }
    }
    
    pkte.ope_cd = TFTP_OP_ERROR;
    if (ni == (UB)(sizeof(el)/sizeof(el[0]))) {
        pkte.err.cd = TFTP_ERC_NODEFINED;
        net_strcpy(pkte.err.buf, (NULL == nodef_msg) ? TFTP_ERS_NODEFINED : nodef_msg);
    }
    else {
        pkte.err.cd = el[ni].cd;
        net_strcpy(pkte.err.buf,  el[ni].msg);
    }
    
    len = (UH)((UINT)net_strlen(pkte.err.buf) + 1U + sizeof(pkte.ope_cd) + sizeof(pkte.err.cd));
    
    return snd_soc(tftp->dat_sid, (VP)&pkte, len);
}

/* Receive TFTP DATA/ACK packet */
static ER tftpd_rcv_dat_ack(T_TFTP_SERVER *tftp, T_TFTP_PKT *pkt, UH pkt_len, UH bnum)
{
    ER ercd;
    T_NODE rmt;
    UH dat_ack;
    
    dat_ack = pkt->ope_cd;
    
    /* Recieve File */
    ercd = rcv_soc(tftp->dat_sid, pkt, pkt_len);
    if (0 < ercd) {
        (void)ref_soc(tftp->dat_sid, SOC_IP_REMOTE, (VP)&rmt);
        
        if ((rmt.ipa != tftp->rmt.ipa) || (rmt.port != tftp->rmt.port)) {
            /* Request opponent is different */
            (void)con_soc(tftp->dat_sid, &rmt, SOC_CLI);
            (void)tftp_snd_err(tftp, TFTP_ERC_UNKNOWN, NULL);
            (void)con_soc(tftp->dat_sid, &tftp->rmt, SOC_CLI);
            ercd = E_NOID;
        }
    }    
    if (0 < ercd) {
        /* Receive data evaluation */
        if (dat_ack == pkt->ope_cd) {
            if (ntohs(pkt->d.dat.blk_num) > bnum) {
                ercd = E_NOSPT;
            }
            else if (ntohs(pkt->d.dat.blk_num) < bnum) {
                /* For communication delay, might receive the previous packet. */
                ercd = (ntohs(pkt->d.dat.blk_num) == (UH)(bnum - 1U)) ? E_OK : E_NOSPT;
            }
            else {
                /* block number equal - do nothing. */
            }
            if (E_NOSPT == ercd) {
                (void)tftp_snd_err(tftp, TFTP_ERC_OPERATE, NULL);
            }
        }
        else if (TFTP_OP_ERROR == pkt->ope_cd) {
            ercd = E_SYS;
        }
        else {
            (void)tftp_snd_err(tftp, TFTP_ERC_OPERATE, NULL);
            ercd = E_NOSPT;
        }
    }        
   
    return ercd;
}

/* Receive of the TFTP data, and write the data to a File */
static ER tftpd_rcv_to_wr(T_TFTP_SERVER *tftp, T_TFTP_PKT *pkt, FILE *fp)
{
    ER ercd;
    enum TFTP_STATUS step;
    enum TFTP_STATUS snd_step;
    UH bnum;
    UH rlen;
    UB loop_end;
    UB retry;
    
    rlen = sizeof(pkt->d.dat.dat);
    bnum = 0U;
    retry = 0U;
    loop_end = 0U;
    snd_step = TFS_UNKNOWN;
    
    for (step = TFS_SEND_ACK; (0U == loop_end);) {
        switch (step) {
        case TFS_RECV_DATA:     /* Receive TFTP Data */
            pkt->ope_cd = TFTP_OP_DATA;
            ercd = tftpd_rcv_dat_ack(tftp, pkt, sizeof(*pkt), bnum + 1U);
            if (0 < ercd) {
                ++bnum;
                retry = 0U;      /* Clear retry count */
                step = TFS_WRITE_FILE;
            }
            else if ((E_NOSPT == ercd) || (E_SYS == ercd)) {
                loop_end = 1U;
            }
            else if (E_NOID == ercd) {
                continue;
            }
            else {
                ++retry;
                if (retry < (UH)TFTP_RETRY_CNT) {
                    step = snd_step;
                }
                else {
                    loop_end = 1U;
                }
            }
            break;            
            
        case TFS_WRITE_FILE:
            ercd -= (ER)(sizeof(pkt->ope_cd) + sizeof(pkt->d.dat.blk_num));
            rlen = (UH)fwrite(pkt->d.dat.dat, 1U, (UINT)ercd, fp);
            if (rlen != (UH)ercd) {
                (void)tftp_snd_err(tftp, TFTP_ERC_DISKFULL, NULL);
                loop_end = 1U;
                ercd = E_NOMEM;
            }
            step = TFS_SEND_ACK;
            break;
            
        case TFS_SEND_ACK:     /* Send TFTP ACK */
            snd_step = TFS_SEND_ACK;
            pkt->ope_cd = TFTP_OP_ACK;
            pkt->d.dat.blk_num = NTOHS(bnum);
            ercd = (ER)(sizeof(pkt->ope_cd) + sizeof(pkt->d.dat.blk_num));
            ercd = snd_soc(tftp->dat_sid, (VP)pkt, (UH)ercd);
            if (0 > ercd) {
                loop_end = 1U;
                break;
            }
            step = TFS_VALID_RECV;
            /* fall through */
            
        case TFS_VALID_RECV:     /* Check for exist of receive data */
            if (rlen == (UH)sizeof(pkt->d.dat.dat)) {
                step = TFS_RECV_DATA;
            }
            else {
                loop_end = 1U;
                ercd = E_OK;
            }
            break;

        default:                /* abnormal branch */
            ercd = E_NOSPT;
            loop_end = 1U;
            break;
        }
    }
    
    return ercd;
}

/* Read of the File data, and send the data to a TFTP */
static ER tftpd_rd_to_snd(T_TFTP_SERVER *tftp, T_TFTP_PKT *pkt, FILE *fp)
{
    VB rbuf[sizeof(pkt->ope_cd) + sizeof(pkt->d.dat.blk_num)];
    ER ercd;
    enum TFTP_STATUS step;
    enum TFTP_STATUS snd_step;
    UH bnum;
    UH rlen;
    UB loop_end;
    UB retry;
    
    bnum = 0U;
    retry = 0U;
    loop_end = 0U;
    snd_step = TFS_UNKNOWN;
    
    for (step = TFS_READ_FILE; (0U == loop_end); ) {
        switch (step) {
        case TFS_READ_FILE:
            ++bnum;
            rlen = (UH)fread(pkt->d.dat.dat, 1U, sizeof(pkt->d.dat.dat), fp);
            pkt->d.dat.blk_num = NTOHS(bnum);
            pkt->ope_cd = TFTP_OP_DATA;
            step = TFS_SEND_DATA;
            /* fall through */
                
        case TFS_SEND_DATA:     /* Send TFTP DATA */
            snd_step = step;
            ercd = (ER)(sizeof(pkt->ope_cd) + sizeof(pkt->d.dat.blk_num) + (UINT)rlen);
            ercd = snd_soc(tftp->dat_sid, (VP)pkt, (UH)ercd);
            if (0 > ercd) {
                loop_end = 1U;
                break;
            }
            step = TFS_RECV_ACK;
            /* fall through */
            
        case TFS_RECV_ACK:      /* Receive TFTP ACK */
            ((T_TFTP_PKT *)rbuf)->ope_cd = TFTP_OP_ACK;
            ercd = tftpd_rcv_dat_ack(tftp, (T_TFTP_PKT *)rbuf, sizeof(rbuf), bnum);
            if (0 < ercd) {
                retry = 0U;      /* Clear retry count */
                step = TFS_VALID_SEND;
            }
            else if ((E_NOSPT == ercd) || (E_SYS == ercd)) {
                loop_end = 1U;
            }
            else if (E_NOID == ercd) {
                continue;
            }
            else {
                ++retry;
                if (retry < (UH)TFTP_RETRY_CNT) {
#ifdef TFTP_IGNORE_FIXSAS
                    step = snd_step;
#else	/* fix "Sorcerer's Apprentice Syndrome" */
					if (E_OK != ercd) {
                    step = snd_step;
					}
#endif
                }
                else {
                    loop_end = 1U;
                }
            }
            break;
            
        case TFS_VALID_SEND:     /* Check for exist of send data */
            if (rlen == (UH)sizeof(pkt->d.dat.dat)) {
                step = TFS_READ_FILE;
            }
            else {
                loop_end = 1U;
                ercd = E_OK;
            }
            break;


        default:                /* abnormal branch */
            ercd = E_NOSPT;
            loop_end = 1U;
            break;
        }
    }
    
    return ercd;
}

static ER server_ini(T_TFTP_SERVER *tftp)
{
    ER ercd;
    if (!tftp) {
        ercd = E_PAR;
    }
    else if ((tftp->req_sid == 0U) || (tftp->dat_sid == 0U)) {
        ercd = E_PAR;
    }
    else {
        ercd = E_OK;
    }
    
    if (E_OK == ercd) {
        /* Set receive device (valid only if there are no associated devices to socket) */
        if (0U != tftp->dev_num) {
            tftp->rmt.port = (UH)TFTP_REQ_PORT;         /* dummy */
            tftp->rmt.ipa = ip_aton("255.255.255.0");   /* dummy */
            tftp->rmt.num = (UB)tftp->dev_num;
            ercd = con_soc(tftp->req_sid, &tftp->rmt, SOC_CLI);
            if (E_OK == ercd) {
                ercd = con_soc(tftp->dat_sid, &tftp->rmt, SOC_CLI);
            }
        }
        else {
            /* Clear receive device */
            (void)cls_soc(tftp->req_sid, 0U);
            (void)cls_soc(tftp->dat_sid, 0U);
        }
    }
    
    return ercd;
}

static ER tftpd_file_open(FILE **fp, const VB *localfile, const VB *mode)
{
    UH len;
    ER ercd;
    VB path[TFTP_FILEPATH_MAX];

    ercd = E_OK;
    
    /* make path */
    len = (UH)(net_strlen(TFTP_ROOT_DIR) + net_strlen(localfile) + 1U);
    if (len > (UH)TFTP_FILEPATH_MAX) {
        *fp = NULL;
        ercd = E_NOMEM;
    }
    else {
        net_strcpy(path, TFTP_ROOT_DIR);
        net_strcat(path, localfile);
    }
    
    /* file open */
    if (E_OK == ercd) {
        *fp = fopen((const char *)path, (const char *)mode);
        if (NULL == *fp) {   /* File not found or Access Deny */
            ercd = E_OACV;
        }
    }
    
    return ercd;
}


ER tftp_server(T_TFTP_SERVER *tftp)
{
    ER ercd;
    VB buf[sizeof(T_TFTP_PKT)];
    VB mode[sizeof("rb")];
    VB *str;
    T_TFTP_PKT *pkt = (T_TFTP_PKT *)buf;
    FILE *fp;
    
    ercd = server_ini(tftp);
    if (E_OK != ercd) {
        return ercd;
    }
    
    while (1) {
        /* Recieve TFTP Request */
        ercd = rcv_soc(tftp->req_sid, buf, sizeof(buf));
        if (0 >= ercd) {
            dly_tsk(TFTP_TMO_NEXT_WAIT);
            continue;
        }
        
        (void)ref_soc(tftp->req_sid, SOC_IP_REMOTE, (VP)&tftp->rmt);
        ercd = con_soc(tftp->dat_sid, &tftp->rmt, SOC_CLI);
        
        /* Check filename length */
        switch (pkt->ope_cd) {
        case TFTP_OP_RRQ:   /* Read request */
        case TFTP_OP_WRQ:   /* Write request */
            str = pkt->d.req.buf;               /* str is filename */
            ercd = (ER)net_strnlen(str, TFTP_FILENAME_MAX);
            if (TFTP_FILENAME_MAX == ercd) {
                fp = NULL;
                (void)tftp_snd_err(tftp, TFTP_ERC_ACCESS, NULL);
                continue;
            }
            str = &str[ercd + 1];   /* str is mode */
            break;
        }
        
        /* Evaluation TFTP Request */
        switch (pkt->ope_cd) {
        case TFTP_OP_RRQ:   /* Read request */
            /* str is mode */
            if (0 == net_strcmp(str, TFTP_MD_ASCII)) {
                net_strcpy(mode, "r");
            }
            else if (0 == net_strcmp(str, TFTP_MD_BINARY)) {
                net_strcpy(mode, "rb");
            } 
            else {
                fp = NULL;
                (void)tftp_snd_err(tftp, TFTP_ERC_ACCESS, NULL);
                break;
            } 
            
            str = pkt->d.req.buf;       /* str is filename */
            ercd = tftpd_file_open(&fp, str, mode);
            if (E_OK != ercd) {
                if (E_OACV == ercd) {   /* File not found */
                    (void)tftp_snd_err(tftp, TFTP_ERC_NOFILE, NULL);
                }
                else {                  /* Unknown Error */
                    (void)tftp_snd_err(tftp, TFTP_ERC_NODEFINED, "Internal error.");
                }
                break;
            }
            break;
            
        case TFTP_OP_WRQ:   /* Write request */
            /* str is mode */
            if (0 == net_strcmp(str, TFTP_MD_ASCII)) {
                net_strcpy(mode, "w");
            }
            else if (0 == net_strcmp(str, TFTP_MD_BINARY)) {
                net_strcpy(mode, "wb");
            } 
            else {
                fp = NULL;
                (void)tftp_snd_err(tftp, TFTP_ERC_ACCESS, NULL);
                break;
            }
            
            str = pkt->d.req.buf;       /* str is filename */
            ercd = tftpd_file_open(&fp, str, mode);
            if (E_OK != ercd) {
                if (E_OACV == ercd) {   /* No write permission */
                    (void)tftp_snd_err(tftp, TFTP_ERC_ACCESS, NULL);
                }
                else {                  /* Unknown Error */
                    (void)tftp_snd_err(tftp, TFTP_ERC_NODEFINED, "Internal error.");
                }
                break;
            }
            break;
            
        default:            /* Other operate code */
            fp = NULL;
            (void)tftp_snd_err(tftp, TFTP_ERC_OPERATE, NULL);
            break;
        }
        
        if (NULL != fp) {
            if ('w' == mode[0]) {
                ercd = tftpd_rcv_to_wr(tftp, pkt, fp);
            }
            else {
                ercd = tftpd_rd_to_snd(tftp, pkt, fp);
            }
            (void)fclose(fp);
        }
        
        /* Socket info clear */
        ercd = abt_soc(tftp->dat_sid, SOC_ABT_ALL);
        ercd = cfg_soc(tftp->dat_sid, SOC_PRT_LOCAL, (VP)TFTP_DATA_PORT);
    }
}

