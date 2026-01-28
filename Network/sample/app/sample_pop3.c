/***************************************************************************
    POP3 Sample (Operating as command in shell)
    Copyright (c)  2014, eForce Co., Ltd. All rights reserved.

    2014-10-30: Created.
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */
#if SAMPLE_ENA_POP3c

#include "pop3_client.h"

static VB pop3_buf[513];
static VB pop3_tmpbuf[1024*4];
static VB pop3_filebuf[1024*4];

static T_POP3_CLIENT   sc;
static T_POP3_MAIL     ml;

#if (0 == SAMPLE_USE_GENSRC)	/* no use configurator */
SID ID_SOC_POP3;

ER sample_pop3c_ini()
{
	ER ercd;
    T_NODE lo_host;

    lo_host.num = SAMPLE_SOCDEV_CLI;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;
    lo_host.port = PORT_ANY;

    /* POP3c */
    ercd = cre_soc(IP_PROTO_TCP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_POP3 = ercd;

    return E_OK;
}
#endif


#if 0	/* POP3 detail command */
/* Command 'pop3_login' */
ER shell_usr_cmd_pop3_login(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    UW ipa;
    
    net_memset(&sc, 0, sizeof(sc));
    
    ercd = dns_get_ipa_opt(SPL_DNS_SERVER, SPL_POP3_SERVER, &ipa);
    if (E_OK != ercd) {
        return ercd;
    }
    
    /* set POP3 settings */
    sc.dev_num = 0;
    sc.ipa  = ipa;
    sc.sid  = ID_TCP_POP3;
    sc.port = SPL_POP3_PORT;
    sc.buf  = pop3_buf;
    sc.len  = sizeof(pop3_buf);
    sc.usr  = SPL_POP3_USER;
    sc.pw   = SPL_POP3_PASS;
    sc.auth_type = SPL_POP3_AUTH;
    
    ercd = pop3_login(&sc);
    if (E_OK != ercd) {
        pop3_quit(&sc);
        return ercd;
    }
    return ercd;    
}

/* Command 'pop3_quit' */
ER shell_usr_cmd_pop3_quit(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    
    ercd = pop3_quit(&sc);
    if (E_OK != ercd) {
        return ercd;
    }
    return ercd;    
}

/* Command 'pop3_cmd' */
ER shell_usr_cmd_pop3_cmd(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    
    switch (argc) {
    case 2:     ercd = pop3_snd_cmd_0(&sc, argv[1]);                    break;
    case 3:     ercd = pop3_snd_cmd_a(&sc, argv[1], argv[2]);           break;
    case 4:     ercd = pop3_snd_cmd_a2(&sc, argv[1], argv[2], argv[3]); break;
    default:    ercd = E_PAR;       break;
    }
    if (ercd != E_PAR) {
        shell_puts(ctrl, "\r\n");
        shell_puts(ctrl, sc.buf);
    }
    
    return ercd;
}

/* Command 'pop3_mcmd' */
ER shell_usr_cmd_pop3_mcmd(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    
    ercd = shell_usr_cmd_pop3_cmd(ctrl, argc, argv);
    if (E_OK == ercd) {
        while (0 == pop3_mline_eod(&sc)) {
            pop3_mline_next(&sc);
            shell_puts(ctrl, sc.buf);
        }        
    }
    
    return (0 <= ercd) ? E_OK : ercd;
}
#endif

/* Command 'pop3' */
ER shell_usr_cmd_pop3(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    UW ipa;
    UW nums, octs;
    T_POP3_FILE sf;
    
    net_memset(&sc, 0, sizeof(sc));
    net_memset(&ml, 0, sizeof(ml));
    net_memset(&sf, 0, sizeof(sf));
    
    ercd = dns_get_ipa_opt(SPL_DNS_SERVER, SPL_POP3_SERVER, &ipa);
    if (E_OK != ercd) {
        return ercd;
    }

    /* set POP3 settings */
    sc.svr.num = 0;
    sc.svr.ipa  = ipa;
    sc.sid  = ID_SOC_POP3;
    sc.svr.port = SPL_POP3_PORT;
    sc.buf  = pop3_buf;
    sc.len  = sizeof(pop3_buf);
    sc.usr  = SPL_POP3_USER;
    sc.pw   = SPL_POP3_PASS;
    sc.auth_type = SPL_POP3_AUTH;
    
    ercd = pop3_login(&sc);
    if (E_OK != ercd) {
        shell_puts(ctrl, "\r\n");
        shell_puts(ctrl, sc.buf);
        pop3_quit(&sc);
        return ercd;
    }
    
    /* get mail status */
    ercd = pop3_cmd_stat(&sc);
    if (E_OK == ercd) {
        pop3_parse_res_i(sc.buf, &nums, &octs);
        shell_puts(ctrl, "\r\n");
        shell_puts(ctrl, sc.buf);
    }
    else {
        shell_puts(ctrl, "\r\n");
        shell_puts(ctrl, sc.buf);
    }
    
    /* get first id message */
    if (0 < nums) {
#if 1   /* save memory */
        sf.type = POP3_FTYP_MEM ;
        sf.buf  = pop3_filebuf;
        sf.len  = sizeof(pop3_filebuf);
#else   /* save filesystem */
        sf.type = POP3_FTYP_FS ;
        sf.buf  = "C:\\mail.dat";
#endif
            
        ercd = pop3_rcv_msg(&sc, 1, &sf);
        if (E_OK == ercd) {
            shell_puts(ctrl, "\r\nreceive_message: \r\n");
            shell_puts(ctrl, sf.buf);
            
            pop3_cnv_mail(&ml, sf.buf);
        }
    }
    
    pop3_quit(&sc);
    
    return ercd;
}

/* Command 'pop3_show' - show bodpy part */
void show_body_part(VP ctrl, T_POP3_PART *part, VB *boundary, UB nest)
{
    T_POP3_PART part_sub;
    VB nbuf[4];
    UB n;
    
    if (!part->body)   return;
    
    /* Display multipart nest */
    for (*nbuf = 0, n = 0; n < nest; ++n)   nbuf[n] = '*';
    nbuf[n] = '\0';
    /* Part Start */
    shell_puts(ctrl, "\r\n");
    shell_puts(ctrl, nbuf);
    shell_puts(ctrl, "-- Part Start--");
    if (boundary) {
        shell_puts(ctrl, "[ ");
        shell_puts(ctrl, boundary);
        shell_puts(ctrl, " ]");
    }
    shell_puts(ctrl, "->");
    
    
    /* Part MIME header */
    shell_puts(ctrl, "\r\n-- Display of part MIME header --");
    if (part->ctype) {    /* Content-Type or boundary */
        shell_puts(ctrl, "\r\n Content-Type : ");
        shell_puts(ctrl, part->ctype);
        
        if (part->ctprm) {    /* Content-Type - name or charset */
            if (POP3_CTP_BOUNDARY & part->ctflg) {
                shell_puts(ctrl, "\r\n  boundary= ");
            }
            else if (POP3_CTP_NAME & part->ctflg) {
                shell_puts(ctrl, "\r\n  name    = ");
                pop3_b64dec_hdr(part->ctprm, part->ctprm, NULL);
            }
            else if (POP3_CTP_CHARSET & part->ctflg){
                shell_puts(ctrl, "\r\n  charset = ");
            }
            shell_puts(ctrl, part->ctprm);
        }
    }
    if (POP3_CTE & part->ctflg) {
        shell_puts(ctrl, "\r\n Content-Transfer-Encoding : ");
        
        switch (POP3_CTE & part->ctflg) {
        case POP3_CTE_7BIT:     shell_puts(ctrl, "7bit");       break;
        case POP3_CTE_8BIT:     shell_puts(ctrl, "7bit");       break;
        case POP3_CTE_BIN:      shell_puts(ctrl, "binary");     break;
        case POP3_CTE_BASE64:   shell_puts(ctrl, "BASE64");     break;
        case POP3_CTE_QTPRT:    shell_puts(ctrl, "quoted-printable");   break;
        default:                shell_puts(ctrl, "Unknown");       break;
        }
    }    
    
    /* Part Body */
    shell_puts(ctrl, "\r\n-- Display of part body --");
    if (POP3_CTP_BOUNDARY & part->ctflg) {    /* nested multipart */
        net_memset(&part_sub, 0, sizeof(part_sub));
        pop3_cnv_part(&part_sub, part->body, part->ctprm);
        show_body_part(ctrl, &part_sub, part->ctprm, nest + 1);
    }
    else {
        shell_puts(ctrl, "\r\n  body:\r\n");
        pop3_b64dec_body(part->body, part->body, (0 == (POP3_CTE_BASE64 & part->ctflg)));
        shell_puts(ctrl, part->body);
    }
    
    /* Part End */
    /* Part Start */
    shell_puts(ctrl, "\r\n");
    shell_puts(ctrl, nbuf);
    shell_puts(ctrl, "<- Part End  --");
    if (boundary) {
        shell_puts(ctrl, "[ ");
        shell_puts(ctrl, boundary);
        shell_puts(ctrl, " ]");
    }
    shell_puts(ctrl, "--");
    
    /* Next Part */
    if (part->next) {
        pop3_cnv_part(&part_sub, part->next, boundary);
        show_body_part(ctrl, &part_sub, boundary, nest);
    }
}

/* Command 'pop3_show' */
ER shell_usr_cmd_pop3_show(VP ctrl, INT argc, VB *argv[])
{
    VB cs[16];
    VB *tmp = pop3_tmpbuf;
    
    if (!ml.from)   return E_OBJ;
    
    /* Show Mail header */
    shell_puts(ctrl, "\r\n-- Display of standard mail header --");
    shell_puts(ctrl, "\r\n From: ");        shell_puts(ctrl, ml.from);
    if (ml.to) {
        shell_puts(ctrl, "\r\n To  : ");    shell_puts(ctrl, ml.to);
    }
    if (ml.cc) {
        shell_puts(ctrl, "\r\n Cc  : ");    shell_puts(ctrl, ml.cc);
    }
    if (ml.date) {
        shell_puts(ctrl, "\r\n Date: ");    shell_puts(ctrl, ml.date);
    }
    if (ml.subject) {
        shell_puts(ctrl, "\r\n Subject: ");
        if (0 < pop3_b64dec_hdr(tmp, ml.subject, cs)) { /* decode */
            shell_puts(ctrl, tmp);
        }
        else {  /* non-decode (not BASE64 encode?) */
            shell_puts(ctrl, ml.subject);
        }
    }
    
    /* Show Mail body */
    net_strcpy(tmp, ml.part.body);
    show_body_part(ctrl, &ml.part, NULL, 0);
    net_strcpy(ml.part.body, tmp);
    
    return E_OK;
}

#endif /* SAMPLE_ENA_POP3c */
