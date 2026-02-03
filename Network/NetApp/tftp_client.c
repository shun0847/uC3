/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    TFTP Client
    Copyright (c)  2013-2019, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.07.12: Created
      2014.12.24: Suppressed warning of Zynq GCC compiler.
      2016.10.03: Corrected the following problems.
        1. Execute static analysis tool to this source.
        2. Support for connection other than the default port.
	  2019.06.13: Fixed "Sorcerer's Apprentice Syndrome".
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_strlib.h"
#include "tftp_client.h"

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

/** Local function definition **/
static ER tftp_snd_err(T_TFTP_CLIENT *tftp, UH err_cd, VB *nodef_msg);
static ER tftpc_rcv_dat_ack(T_TFTP_CLIENT *tftp, T_TFTP_PKT *pkt, UH pkt_len, UH bnum);
static UH make_req_pkt(T_TFTP_PKT *pkt, const VB *rmt_file, UB asc);

/*----------------------------------------------------------------*/
/* Send TFTP error packet */
static ER tftp_snd_err(T_TFTP_CLIENT *tftp, UH err_cd, VB *nodef_msg)
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
    
    len = (UH)(sizeof(pkte.ope_cd) + sizeof(pkte.err.cd));
    len += (UH)(net_strlen(pkte.err.buf) + 1U);
    
    return snd_soc(tftp->dat_sid, (VP)&pkte, len);
}

/* Receive TFTP DATA/ACK packet */
static ER tftpc_rcv_dat_ack(T_TFTP_CLIENT *tftp, T_TFTP_PKT *pkt, UH pkt_len, UH bnum)
{
    ER ercd;
    T_NODE rmt;
    UH dat_ack;
    
    dat_ack = pkt->ope_cd;
    
    /* Recieve File */
    ercd = rcv_soc(tftp->dat_sid, pkt, pkt_len);
    if (0 < ercd) {
        (void)ref_soc(tftp->dat_sid, SOC_IP_REMOTE, (VP)&rmt);
        
        if ((rmt.ipa == tftp->rmt.ipa) && (tftp->port == tftp->rmt.port)) {
            tftp->rmt.port = rmt.port;
            (void)con_soc(tftp->dat_sid, &tftp->rmt, SOC_CLI);
        }
        else if ((rmt.ipa != tftp->rmt.ipa) || (rmt.port != tftp->rmt.port)) {
            /* Request opponent is different */
            (void)con_soc(tftp->dat_sid, &rmt, SOC_CLI);
            (void)tftp_snd_err(tftp, TFTP_ERC_UNKNOWN, NULL);
            (void)con_soc(tftp->dat_sid, &tftp->rmt, SOC_CLI);
            ercd = E_NOID;
        }
        else {
            /* do nothing. */
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

/* Make TFTP RRQ/WRQ Packet */
static UH make_req_pkt(T_TFTP_PKT *pkt, const VB *rmt_file, UB asc)
{
    VB *str;
    UH len;
    
    str = pkt->d.req.buf;
    net_strcpy(str, rmt_file);
    len = (UH)net_strlen(str);
    str = &pkt->d.req.buf[len + 1];
    net_strcpy(str, (0U == asc) ? TFTP_MD_BINARY : TFTP_MD_ASCII);
    len += (UH)(net_strlen(str) + 2U);
    len += (UH)sizeof(pkt->ope_cd);
    
    return len;
}

static ER tftpc_ini(T_TFTP_CLIENT *tftp, const VB *lo_file, const VB *rmt_file)
{
    ER ercd;

    /* Error Check */
    if (NULL == tftp) {
        ercd = E_PAR;
    }
    else if ((UB)NET_DEV_MAX < tftp->rmt.num) {
        ercd = E_PAR;
    }
    else if ((0U == tftp->rmt.ipa) || (0U == tftp->dat_sid)) {
        ercd = E_PAR;
    }
    else if ((NULL == lo_file) || (NULL == rmt_file)) {
        ercd = E_PAR;
    }
    else {
        /* Value Check */
        if (DEV_ANY == tftp->rmt.num) {
            ++tftp->rmt.num;
        }
        if (0U == tftp->port) {
            tftp->port = TFTP_REQ_PORT;
        }

        /* Socket setting clear */
        tftp->rmt.port = tftp->port;
        (void)abt_soc(tftp->dat_sid, SOC_ABT_ALL);
        (void)cfg_soc(tftp->dat_sid, SOC_PRT_LOCAL, (VP)TFTP_DATA_PORT);
        ercd = con_soc(tftp->dat_sid, &tftp->rmt, SOC_CLI);
    }
    
    return ercd;
}

/* TFTP GET */
ER tftp_get_file(T_TFTP_CLIENT *tftp, const VB *lo_file, const VB *rmt_file)
{
    VB buf[sizeof(T_TFTP_PKT)]; /* 524 ? */
    T_TFTP_PKT *pkt = (T_TFTP_PKT *)buf;
    FILE *fp;
    ER ercd;
    enum TFTP_STATUS step;
    enum TFTP_STATUS snd_step;
    UH bnum;
    UH rlen;
    UB loop_end;
    UB retry;

    /* check parameter */
    ercd = tftpc_ini(tftp, lo_file, rmt_file);
    if (E_OK == ercd) {
        /* Create File */
        fp = fopen((const char *)lo_file, (const char *)((0U == tftp->asc) ? "wb" : "w"));
        if (NULL == fp) {   /* No write permission */
            ercd = E_OACV;
        }
    }
    if (E_OK != ercd) {
        return ercd;
    }
    
    rlen = 0U;
    bnum = 0U;
    retry = 0U;
    loop_end = 0U;
    snd_step = TFS_UNKNOWN;
    
    for (step = TFS_SEND_REQ; (0U == loop_end); ) {
        switch (step) {
        case TFS_RECV_DATA:     /* Receive TFTP Data */
            pkt->ope_cd = TFTP_OP_DATA;
            ercd = tftpc_rcv_dat_ack(tftp, pkt, sizeof(*pkt), bnum + 1U);
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
            pkt->d.dat.blk_num = ntohs(bnum);
            ercd = (ER)(sizeof(pkt->ope_cd) + sizeof(pkt->d.dat.blk_num));
            ercd = snd_soc(tftp->dat_sid, (VP)pkt, (UH)ercd);
            if (0 > ercd) {
                loop_end = 1U;
                break;
            }
            step = TFS_VALID_RECV;
            /* fall through */
            
        case TFS_VALID_RECV:     /* Check for exist of receive data */
            if (rlen == TFTP_BLKS_LEN) {
                step = TFS_RECV_DATA;
            }
            else {
                loop_end = 1U;
                ercd = E_OK;
            }
            break;
            
        case TFS_SEND_REQ:     /* Send TFTP RRQ */
            snd_step = step;
            pkt->ope_cd = TFTP_OP_RRQ;
            ercd = (ER)make_req_pkt(pkt, rmt_file, tftp->asc);
            ercd = snd_soc(tftp->dat_sid, (VP)pkt, (UH)ercd);
            if (0 > ercd) {
                loop_end = 1U;
                break;
            }
            step = TFS_RECV_DATA;
            break;

        default:                /* abnormal branch */
            ercd = E_NOSPT;
            loop_end = 1U;
            break;
        }
    }
    
    (void)fclose(fp);
    
    return ercd;
}

/* TFTP PUT */
ER tftp_put_file(T_TFTP_CLIENT *tftp, const VB *lo_file, const VB *rmt_file)
{
    VB buf[sizeof(T_TFTP_PKT)]; /* 524 ? */
    T_TFTP_PKT *pkt = (T_TFTP_PKT *)buf;
    VB rbuf[sizeof(pkt->ope_cd) + sizeof(pkt->d.dat.blk_num)];
    T_TFTP_PKT *rpkt;
    FILE *fp;
    ER ercd;
    enum TFTP_STATUS step;
    enum TFTP_STATUS snd_step;
    UH bnum;
    UH rlen;
    UB loop_end;
    UB retry;

    /* check parameter */
    ercd = tftpc_ini(tftp, lo_file, rmt_file);
    if (E_OK == ercd) {
        /* Open File */
        fp = fopen((const char *)lo_file, (const char *)((0U == tftp->asc) ? "rb" : "r"));
        if (NULL == fp) {   /* File not found */
            ercd = E_OACV;
        }
    }
    if (E_OK != ercd) {
        return ercd;
    }
    
    rlen = 0U;
    bnum = 0U;
    retry = 0U;
    loop_end = 0U;
    snd_step = TFS_UNKNOWN;
    
    for (step = TFS_SEND_REQ; (0U == loop_end); ) {
        switch (step) {
        case TFS_READ_FILE:
            ++bnum;
            rlen = (UH)fread(pkt->d.dat.dat, 1U, TFTP_BLKS_LEN, fp);
            pkt->d.dat.blk_num = ntohs(bnum);
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
            rpkt = (T_TFTP_PKT *)rbuf;
            rpkt->ope_cd = TFTP_OP_ACK;
            ercd = tftpc_rcv_dat_ack(tftp, rpkt, sizeof(rbuf), bnum);
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
            
        case TFS_VALID_SEND:    /* Check for exist of send data */
            if (rlen == TFTP_BLKS_LEN) {
                step = TFS_READ_FILE;
            }
            else {
                loop_end = 1U;
                ercd = E_OK;
            }
            break;
            
        case TFS_SEND_REQ:     /* Send TFTP WRQ */
            snd_step = step;
            rlen = TFTP_BLKS_LEN;
            pkt->ope_cd = TFTP_OP_WRQ;
            ercd = (ER)make_req_pkt(pkt, rmt_file, tftp->asc);
            ercd = snd_soc(tftp->dat_sid, (VP)pkt, (UH)ercd);
            if (0 > ercd) {
                loop_end = 1U;
                break;
            }
            step = TFS_RECV_ACK;
            break;

        default:                /* abnormal branch */
            ercd = E_NOSPT;
            loop_end = 1U;
            break;
        }
    }
    
    (void)fclose(fp);
    
    return ercd;
}
