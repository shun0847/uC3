/****************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel common definitions

    Copyright (c)  2008-2018, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.19: Created.
            2008.10.24: Corrected the error in writing.
            2008.11.27: Corrected the function definitions.
            2009.07.20: Modified the configuration of object max ID.
            2009.09.28: Modified for the multi-core extension.
            2009.11.11: Corrected the function code.
            2010.12.29: Added the function code.
            2013.09.20: Modified TSZ_MPF macro.
            2013.09.30: Supported the YDC SystemMacroTrace.
            2014.08.19: Supported the CubeGEAR Trace.
            2015.02.19: Added "C" linkage macro.
            2015.08.28: Modified TSZ_MBF, TSZ_MPL macro.
            2016.01.06: Fixed in accordance with IPA_C-M-4.7.1b-1.
            2017.01.26: Fixed the IPA warnings.
            2018.05.23: move function code to uC3fncode.h.
 ****************************************************************************/

#ifndef _KERNEL_H_
#define _KERNEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "itron.h"

#ifdef _UC3SYS_H_
#define EXTERN
#else
#define EXTERN  extern
#endif

/***********************************
        パケット形式
 ***********************************/

/* タスク管理機能 */

typedef struct t_ctsk {
    ATR         tskatr;         /* タスク属性                               */
    VP_INT      exinf;          /* タスクの拡張情報                         */
    FP          task;           /* タスクの起動番地                         */
    PRI         itskpri;        /* タスクの起動時優先度                     */
    SIZE        stksz;          /* タスクのスタックサイズ                   */
    VP          stk;            /* タスクのスタック領域の先頭番地           */
    VB const    *name;          /* タスクの名称                             */
} T_CTSK;

typedef struct t_rtsk {
    STAT        tskstat;        /* タスク状態                               */
    PRI         tskpri;         /* タスクの現在優先度                       */
    PRI         tskbpri;        /* タスクのベース優先度                     */
    STAT        tskwait;        /* 待ち要因                                 */
    ID          wobjid;         /* 待ち対象のオブジェクトのID番号           */
    TMO         lefttmo;        /* タイムアウトするまでの時間               */
    UINT        actcnt;         /* 起動要求キューイング数                   */
    UINT        wupcnt;         /* 起床要求キューイング数                   */
    UINT        suscnt;         /* 強制待ち要求ネスト数                     */
} T_RTSK;

typedef struct t_rtst {
    STAT        tskstat;        /* タスク状態                               */
    STAT        tskwait;        /* 待ち要因                                 */
} T_RTST;

/* タスク例外処理機能 */

typedef struct t_dtex {
    ATR         texatr;         /* タスク例外処理ルーチン属性               */
    FP          texrtn;         /* タスク例外処理ルーチンの起動番地         */
} T_DTEX;

typedef struct t_rtex {
    STAT        texstat;        /* タスク例外処理の状態                     */
    TEXPTN      pndptn;         /* 保留例外要因                             */
} T_RTEX;

/* 同期・通信機能（セマフォ） */

typedef struct t_csem {
    ATR         sematr;         /* セマフォ属性                             */
    UINT        isemcnt;        /* セマフォの資源数の初期値                 */
    UINT        maxsem;         /* セマフォの最大資源数                     */
    VB const    *name;          /* セマフォの名称                           */
}T_CSEM;

typedef struct t_rsem {
    ID          wtskid;         /* セマフォの待ち行列の先頭のタスクのID番号 */
    UINT        semcnt;         /* セマフォの現在の資源数                   */
} T_RSEM;

/* 同期・通信機能（イベントフラグ） */

typedef struct t_cflg {
    ATR         flgatr;         /* イベントフラグ属性                       */
    FLGPTN      iflgptn;        /* イベントフラグのビットパターンの初期値   */
    VB const    *name;          /* イベントフラグの名称                     */
} T_CFLG;

typedef struct t_rflg {
    ID          wtskid;         /* イベントフラグの待ち行列の先頭のタスクのID番号   */
    FLGPTN      flgptn;         /* イベントフラグの現在のビットパターン             */
} T_RFLG;

/* 同期・通信機能（データキュー） */

typedef struct t_cdtq {
    ATR         dtqatr;         /* データキュー属性                         */
    UINT        dtqcnt;         /* データキューの容量（データの個数）       */
    VP          dtq;            /* データキュー領域の先頭番地               */
    VB const    *name;          /* データキューの名称                       */
} T_CDTQ;

typedef struct t_rdtq {
    ID          stskid;         /* データキューの送信待ち行列の先頭のタスクのID番号 */
    ID          rtskid;         /* データキューの受信待ち行列の先頭のタスクのID番号 */
    UINT        sdtqcnt;        /* データキューに入っているデータの数               */
} T_RDTQ;

/* 同期・通信機能（メールボックス） */

typedef struct t_cmbx {
    ATR         mbxatr;         /* メールボックス属性                               */
    PRI         maxmpri;        /* 送信されるメッセージの優先度の最大値             */
    VP          mprihd;         /* 優先度別のメッセージキューヘッダの領域の先頭番地 */
    VB const    *name;          /* メールボックスの名称                             */
} T_CMBX;

typedef struct t_rmbx {
    ID          wtskid;         /* メールボックスの待ち行列の先頭のタスクのID番号       */
    struct t_msg    *pk_msg;    /* メッセージキューの先頭のメッセージパケットの先頭番地 */
} T_RMBX;

typedef struct t_msg {
    struct t_msg    *msgque;    /* メッセージヘッダ                         */
} T_MSG;

typedef struct t_msg_pri {
    struct t_msg_pri    *msgque;/* メッセージヘッダ                         */
    PRI                 msgpri; /* メッセージ優先度                         */
} T_MSG_PRI;

/* 拡張同期・通信機能（ミューテックス） */

typedef struct t_cmtx {
    ATR         mtxatr;         /* ミューテックス属性                       */
    PRI         ceilpri;        /* ミューテックスの上限優先度               */
    VB const    *name;          /* ミューテックスの名称                     */
} T_CMTX;

typedef struct t_rmtx {
    ID          htskid;         /* ミューテックスをロックしているタスクのID番号     */
    ID          wtskid;         /* ミューテックスの待ち行列の先頭のタスクのID番号   */
} T_RMTX;

/* 拡張同期・通信機能（メッセージバッファ） */

typedef struct t_cmbf {
    ATR         mbfatr;         /* メッセージバッファ属性                       */
    UINT        maxmsz;         /* メッセージの最大サイズ（バイト数）           */
    SIZE        mbfsz;          /* メッセージバッファ領域のサイズ（バイト数）   */
    VP          mbf;            /* メッセージバッファ領域の先頭番地             */
    VB const    *name;          /* メッセージバッファの名称                     */
} T_CMBF;

typedef struct t_rmbf {
    ID          stskid;         /* メッセージバッファの送信待ち行列の先頭のタスクのID番号   */
    ID          rtskid;         /* メッセージバッファの受信待ち行列の先頭のタスクのID番号   */
    UINT        smsgcnt;        /* メッセージバッファに入っているメッセージの数             */
    SIZE        fmbfsz;         /* メッセージバッファ領域の空き領域のサイズ（バイト数）     */
} T_RMBF;

/* 拡張同期・通信機能（ランデブポート） */

typedef struct t_cpor {
    ATR         poratr;         /* ランデブポート属性                       */
    UINT        maxcmsz;        /* 呼出しメッセージの最大サイズ（バイト数） */
    UINT        maxrmsz;        /* 返答メッセージの最大サイズ（バイト数）   */
    VB const    *name;          /* ランデブポートの名称                     */
} T_CPOR;

typedef struct t_rpor {
    ID          ctskid;         /* ランデブポートの呼出し待ち行列の先頭のタスクのID番号 */
    ID          atskid;         /* ランデブポートの受付待ち行列の先頭のタスクのID番号   */
} T_RPOR;

typedef struct t_rrdv {
    ID          wtskid;         /* ランデブ終了待ち状態のタスクのID番号     */
} T_RRDV;

/* メモリプール管理機能（固定長メモリプール） */

typedef struct t_cmpf {
    ATR         mpfatr;         /* 固定長メモリプール属性                   */
    UINT        blkcnt;         /* 獲得できるメモリブロック数（個数）       */
    UINT        blksz;          /* メモリブロックのサイズ（バイト数）       */
    VP          mpf;            /* 固定長メモリプール領域の先頭番地         */
    VB const    *name;          /* 固定長メモリプールの名称                 */
} T_CMPF;

typedef struct t_rmpf {
    ID          wtskid;         /* 固定長メモリプールの待ち行列の先頭のタスクのID番号   */
    UINT        fblkcnt;        /* 固定長メモリプールの空きメモリブロック数（個数）     */
} T_RMPF;

/* メモリプール管理機能（可変長メモリプール） */

typedef struct t_cmpl {
    ATR         mplatr;         /* 可変長メモリプール属性                       */
    SIZE        mplsz;          /* 可変長メモリプール領域のサイズ（バイト数）   */
    VP          mpl;            /* 可変長メモリプール領域の先頭番地             */
    VB const    *name;          /* 可変長メモリプールの名称                     */
} T_CMPL;

typedef struct t_rmpl {
    ID          wtskid;         /* 可変長メモリプールの待ち行列の先頭のタスクのID番号   */
    SIZE        fmplsz;         /* 可変長メモリプールの空き領域の合計サイズ（バイト数） */
    UINT        fblksz;         /* 過ぎに獲得可能な最大メモリブロックサイズ（バイト数） */
} T_RMPL;

/* 時間管理機能（周期ハンドラ） */

typedef struct t_ccyc {
    ATR         cycatr;         /* 周期ハンドラ属性                         */
    VP_INT      exinf;          /* 周期ハンドラの拡張情報                   */
    FP          cychdr;         /* 周期ハンドラの起動番地                   */
    RELTIM      cyctim;         /* 周期ハンドラの起動周期                   */
    RELTIM      cycphs;         /* 周期ハンドラの起動位相                   */
    VB const    *name;          /* 周期ハンドラの名称                       */
} T_CCYC;

typedef struct t_rcyc {
    STAT        cycstat;        /* 周期ハンドラの動作状態                   */
    RELTIM      lefttim;        /* 周期ハンドラを次に起動する時刻までの時間 */
} T_RCYC;

/* 時間管理機能（アラームハンドラ） */

typedef struct t_calm {
    ATR         almatr;         /* アラームハンドラ属性                     */
    VP_INT      exinf;          /* アラームハンドラの拡張情報               */
    FP          almhdr;         /* アラームハンドラの起動番地               */
    VB const    *name;          /* アラームハンドラの名称                   */
} T_CALM;

typedef struct t_ralm {
    STAT        almstat;        /* アラームハンドラの動作状態               */
    RELTIM      lefttim;        /* アラームハンドラの起動時刻までの時間     */
} T_RALM;

/* 時間管理機能（オーバランハンドラ） */

typedef struct t_dovr {
    ATR         ovratr;         /* オーバランハンドラ属性                   */
    FP          ovrhdr;         /* オーバランハンドラの起動番地             */
} T_DOVR;

typedef struct t_rovr {
    STAT        ovrstat;        /* オーバランハンドラの動作状態             */
    OVRTIM      leftotm;        /* 残りのプロセッサ時間                     */
} T_ROVR;

/* 独自機能（デバイスドライバ） */

typedef struct t_cdev {
    VP          ctrblk;         /* 制御情報の先頭番地                       */
    FP          devhdr;         /* デバイスドライバの起動番地               */
    VB const    *name;          /* デバイスドライバの名称                   */
} T_CDEV;

/* システム管理機能 */

typedef struct t_csys {
    UH          tskpri_max;     /* タスク優先度の最大値                     */
    UH          tskid_max;      /* タスクIDの最大値                         */
    UH          semid_max;      /* セマフォIDの最大値                       */
    UH          flgid_max;      /* イベントフラグIDの最大値                 */
    UH          dtqid_max;      /* データキューIDの最大値                   */
    UH          mbxid_max;      /* メールボックスIDの最大値                 */
    UH          mtxid_max;      /* ミューテックスIDの最大値                 */
    UH          mbfid_max;      /* メッセージバッファIDの最大値             */
    UH          porid_max;      /* ランデブポートIDの最大値                 */
    UH          mpfid_max;      /* 固定長メモリプールIDの最大値             */
    UH          mplid_max;      /* 可変長メモリプールIDの最大値             */
    UH          almid_max;      /* アラームハンドラIDの最大値               */
    UH          cycid_max;      /* 周期ハンドラIDの最大値                   */
    UH          isrid_max;      /* 割込みサービスルーチンIDの最大値         */
    UH          devid_max;      /* デバイスドライバIDの最大値               */
    UH          tick;           /* チック時間（㍉秒）                       */
    UH          ssb_num;        /* システムサービスブロックの生成個数       */
    VP          sysmem_top;     /* システムメモリの先頭番地                 */
    VP          sysmem_end;     /* システムメモリのサイズ                   */
    VP          stkmem_top;     /* スタック用メモリの先頭番地               */
    VP          stkmem_end;     /* スタック用メモリのサイズ                 */
    VP          mplmem_top;     /* メモリプール用メモリの先頭番地           */
    VP          mplmem_end;     /* メモリプール用メモリのサイズ             */
    FP          ctrtim;         /* タイマ制御関数の番地                     */
    FP          sysidl;         /* アイドル関数の番地                       */
    FP          inistk;         /* スタック初期化関数の番地                 */
    FP          trace;          /* トレース機能初期化関数の番地             */
    FP          agent;          /* エージェント機能初期化関数の番地         */
} T_CSYS;

typedef struct t_rsys {
    SIZE        fsyssz;         /* システムメモリの空き領域の合計サイズ     */
    SIZE        fstksz;         /* スタックメモリの空き領域の合計サイズ     */
    SIZE        fmplsz;         /* プールメモリの空き領域の合計サイズ       */
    UH          utskid;         /* 生成済みタスクIDの個数                   */
    UH          usemid;         /* 生成済みセマフォIDの個数                 */
    UH          uflgid;         /* 生成済みイベントフラグIDの個数           */
    UH          udtqid;         /* 生成済みデータキューIDの個数             */
    UH          umbxid;         /* 生成済みメールボックスIDの個数           */
    UH          umtxid;         /* 生成済みミューテックスIDの個数           */
    UH          umbfid;         /* 生成済みメッセージバッファIDの個数       */
    UH          uporid;         /* 生成済みランデブポートIDの個数           */
    UH          umpfid;         /* 生成済み固定長メモリプールIDの個数       */
    UH          umplid;         /* 生成済み可変長メモリプールIDの個数       */
    UH          ualmid;         /* 生成済みアラームハンドラIDの個数         */
    UH          ucycid;         /* 生成済み周期ハンドラIDの個数             */
    UH          uisrid;         /* 生成済み割込みサービスルーチンIDの個数   */
    UH          ssbcnt;         /* SSBの最小個数                            */
} T_RSYS;

/* 割込み管理機能 */

typedef struct t_dinh {
    ATR         inhatr;         /* 割込みハンドラ属性                       */
    FP          inthdr;         /* 割込みハンドラの起動番地                 */
    IMASK       imask;          /* 割込みハンドラの割込みマスクレベル       */
} T_DINH;

typedef struct t_cisr {
    ATR         isratr;         /* 割込みサービスルーチン属性                   */
    VP_INT      exinf;          /* 割込みサービスルーチンの拡張情報             */
    INTNO       intno;          /* 割込みサービスルーチンを付加する割込み番号   */
    FP          isr;            /* 割込みサービスルーチンの起動番地             */
    IMASK       imask;          /* 割込みサービスルーチンの割込みマスクレベル   */
} T_CISR;

typedef struct t_risr {
    INTNO       intno;          /* 割込みサービスルーチンを付加した割込み番号   */
    FP          isr;            /* 割込みサービスルーチンの起動番地             */
} T_RISR;

/* システム構成管理機能 */

typedef struct t_dexc {
    ATR         excatr;         /* CPU例外ハンドラの属性                    */
    FP          exchdr;         /* CPU例外ハンドラの起動番地                */
} T_DEXC;

typedef struct t_rcfg {
    UH          tskpri_max;     /* タスク優先度の最大値                     */
    UH          tskid_max;      /* タスクIDの最大値                         */
    UH          semid_max;      /* セマフォIDの最大値                       */
    UH          flgid_max;      /* イベントフラグIDの最大値                 */
    UH          dtqid_max;      /* データキューIDの最大値                   */
    UH          mbxid_max;      /* メールボックスIDの最大値                 */
    UH          mtxid_max;      /* ミューテックスIDの最大値                 */
    UH          mbfid_max;      /* メッセージバッファIDの最大値             */
    UH          porid_max;      /* ランデブポートIDの最大値                 */
    UH          mpfid_max;      /* 固定長メモリプールIDの最大値             */
    UH          mplid_max;      /* 可変長メモリプールIDの最大値             */
    UH          almid_max;      /* アラームハンドラIDの最大値               */
    UH          cycid_max;      /* 周期ハンドラIDの最大値                   */
    UH          isrid_max;      /* 割込みサービスルーチンIDの最大値         */
    UH          devid_max;      /* デバイスドライバIDの最大値               */
    UH          tick;           /* チック時間（㍉秒）                       */
    UH          ssb_cnt;        /* システムサービスブロックの生成個数       */
} T_RCFG;

typedef struct t_rver {
    UH          maker;          /* カーネルのメーカコード                   */
    UH          prid;           /* カーネルの識別番号                       */
    UH          spver;          /* ITRON仕様のバージョン番号                */
    UH          prver;          /* カーネルのバージョン番号                 */
    UH          prno[4];        /* カーネルの製品情報                       */
} T_RVER;

/***********************************
        定数
 ***********************************/

/* オブジェクト属性 */

#define TA_NULL     0U          /* オブジェクト属性を指定しない             */
#define TA_HLNG     0x00U       /* 高級言語のインタフェース                 */
#define TA_ASM      0x01U       /* アセンブラ言語のインタフェース           */
#define TA_ACT      0x02U       /* タスクを起動された状態で生成             */
#define TA_RSTR     0x04U       /* 制約タスク                               */

#define TA_AUX      0x10U       /* 予備属性                                 */
#define TA_DSP      0x20U       /* DSP許可属性                              */
#define TA_FPU      0x40U       /* FPU許可属性                              */
#define TA_VPU      0x80U       /* VPU許可属性                              */

#define TA_TFIFO    0x00U       /* FIFO順の待ち行列                         */
#define TA_TPRI     0x01U       /* タスク優先順の待ち行列                   */

#define TA_MFIFO    0x00U       /* FIFO順のメッセージキュー                 */
#define TA_MPRI     0x02U       /* 優先度順のメッセージキュー               */

#define TA_WSGL     0x00U       /* イベントフラグの複数タスク待ちを禁止     */
#define TA_WMUL     0x02U       /* イベントフラグの複数タスク待ちを許可     */
#define TA_CLR      0x04U       /* 待ち解除時のイベントフラグのクリア       */

#define TA_INHERIT  0x02U       /* 優先度継承プロトコル                     */
#define TA_CEILING  0x03U       /* 優先度上限プロトコル                     */

#define TA_STA      0x02U       /* 周期ハンドラを動作している状態で生成     */
#define TA_PHS      0x04U       /* 周期ハンドラの位相を保存                 */

/* タイムアウト指定 */

#define TMO_POL     0           /* ポーリング                               */
#define TMO_FEVR    -1          /* 永久待ち                                 */

/* システムコールの動作モード */

#define TWF_ANDW    0x00U       /* イベントフラグのAND待ち                  */
#define TWF_ORW     0x01U       /* イベントフラグのOR待ち                   */

/* オブジェクトの状態 */

#define TTS_RUN     0x01U       /* 実行状態                                 */
#define TTS_RDY     0x02U       /* 実行可能状態                             */
#define TTS_WAI     0x04U       /* 待ち状態                                 */
#define TTS_SUS     0x08U       /* 強制待ち状態                             */
#define TTS_WAS     0x0CU       /* 二重待ち状態                             */
#define TTS_DMT     0x10U       /* 休止状態                                 */

#define TTW_SLP     0x0001U     /* 起床待ち状態                             */
#define TTW_DLY     0x0002U     /* 時間経過待ち状態                         */
#define TTW_SEM     0x0004U     /* セマフォ資源の獲得待ち状態               */
#define TTW_FLG     0x0008U     /* イベントフラグの待ち状態                 */
#define TTW_SDTQ    0x0010U     /* データキューへの送信待ち状態             */
#define TTW_RDTQ    0x0020U     /* データキューからの受信待ち状態           */
#define TTW_MBX     0x0040U     /* メールボックスからの受信待ち状態         */
#define TTW_MTX     0x0080U     /* ミューテックスのロック待ち状態           */
#define TTW_SMBF    0x0100U     /* メッセージバッファへの送信待ち状態       */
#define TTW_RMBF    0x0200U     /* メッセージバッファからの受信待ち状態     */
#define TTW_CAL     0x0400U     /* ランデブの呼び出し待ち状態               */
#define TTW_ACP     0x0800U     /* ランデブの受付待ち状態                   */
#define TTW_RDV     0x1000U     /* ランデブの終了待ち状態                   */
#define TTW_MPF     0x2000U     /* 固定長メモリブロックの獲得待ち状態       */
#define TTW_MPL     0x4000U     /* 可変長メモリブロックの獲得待ち状態       */

#define TTEX_ENA    0x00U       /* タスク例外ハンドラの許可状態             */
#define TTEX_DIS    0x01U       /* タスク例外ハンドラの禁止状態             */

#define TCYC_STP    0x00U       /* 周期ハンドラが動作していない             */
#define TCYC_STA    0x01U       /* 周期ハンドラが動作している               */

#define TALM_STP    0x00U       /* アラームハンドラが動作していない         */
#define TALM_STA    0x01U       /* アラームハンドラが動作している           */

#define TOVR_STP    0x00U       /* 上限プロセッサ時間が設定されていない     */
#define TOVR_STA    0x01U       /* 上限プロセッサ時間が設定されている       */

/* その他の定数 */

#define TSK_SELF    0           /* 自タスク指定                             */
#define TSK_NONE    0           /* 該当するタスクがない                     */

#define TPRI_SELF   0           /* 自タスクのベース優先度の指定             */
#define TPRI_INI    0           /* タスクの起動時優先度の指定               */

#define TKERNEL_MAKER   0x0000U /* カーネルのメーカコード                   */
#define TKERNEL_SPVER   0x5403U /* ITRON仕様のバージョン番号                */

/***********************************
        構成定数とマクロ
 ***********************************/

/* オブジェクト・優先度の範囲 */

#define TMIN_TPRI   1           /* タスク優先度の最小値                     */
#define TMAX_TPRI   31          /* タスク優先度の最大値                     */

#define TMIN_MPRI   1           /* メッセージ優先度の最小値                 */
#define TMAX_MPRI   31          /* メッセージ優先度の最大値                 */

#define TMIN_OBJ    1           /* タスク以外のオブジェクトIDの最小値       */
#ifndef TMAX_OBJ
#define TMAX_OBJ    999         /* タスク以外のオブジェクトIDの最大値       */
#endif

#define TMIN_TSK    1           /* タスクIDの最小値                         */
#ifndef TMAX_TSK
#define TMAX_TSK    255         /* タスクIDの最大値                         */
#endif

/* キューイング/ネスト回数の最大値 */

#define TMAX_ACTCNT 999U        /* タスクの起動要求キューイング数の最大値   */
#define TMAX_WUPCNT 999U        /* タスクの起床要求キューイング数の最大値   */
#define TMAX_SUSCNT 999U        /* タスクの強制待ち要求ネスト数の最大値     */
#define TMAX_MAXSEM 999U        /* セマフォの最大資源数の最大値             */

/* ビットパターンのビット数 */

#define TBIT_TEXPTN _kernel_INT_BIT
#define TBIT_FLGPTN _kernel_INT_BIT
#define TBIT_RDVPTN _kernel_INT_BIT

/* 必要なメモリ領域のサイズ */

#define TSZ_DTQ(i)      ((i)*sizeof(VP_INT))
#define TSZ_MPRIHD(i)   ((i)*sizeof(VP)*2)
#define TSZ_MBF(i,j)    ((i)*(((j)+_kernel_ALIGN_SIZE-1)&(~(_kernel_ALIGN_SIZE-1))))

#define TSZ_MPF(i,j)    ((i)*(((j)+(_kernel_ALIGN_SIZE-1))&(~(_kernel_ALIGN_SIZE-1))))
#define TSZ_MPL(i,j)    ((i)*(((j)+_kernel_ALIGN_SIZE+_kernel_ALIGN_SIZE-1)&(~(_kernel_ALIGN_SIZE-1))))


/***********************************
        エラーコード
 ***********************************/

#define E_SYS       -5          /* 0xFFFB: システムエラー                   */
#define E_NOSPT     -9          /* 0xFFF7: 未サポートエラー                 */
#define E_RSFN      -10         /* 0xFFF6: 予約機能コード                   */
#define E_RSATR     -11         /* 0xFFF5: 予約属性                         */
#define E_PAR       -17         /* 0xFFEF: パラメータエラー                 */
#define E_ID        -18         /* 0xFFEE: 不正ID番号                       */
#define E_CTX       -25         /* 0xFFE7: コンテキストエラー               */
#define E_MACV      -26         /* 0xFFE6: メモリアクセス違反               */
#define E_OACV      -27         /* 0xFFE5: オブジェクトアクセス違反         */
#define E_ILUSE     -28         /* 0xFFE4: サービスコール不正使用           */
#define E_NOMEM     -33         /* 0xFFDF: メモリ不足                       */
#define E_NOID      -34         /* 0xFFDE: ID番号不足                       */
#define E_OBJ       -41         /* 0xFFD7: オブジェクト状態エラー           */
#define E_NOEXS     -42         /* 0xFFD6: オブジェクト未生成               */
#define E_QOVR      -43         /* 0xFFD5: キューイングオーバフロー         */
#define E_RLWAI     -49         /* 0xFFCF: 待ち状態の強制解除               */
#define E_TMOUT     -50         /* 0xFFCE: ポーリング失敗またはタイムアウト */
#define E_DLT       -51         /* 0xFFCD: 待ちオブジェクトの削除           */
#define E_CLS       -52         /* 0xFFCC: 待ちオブジェクトの状態変化       */
#define E_WBLK      -57         /* 0xFFC7: ノンブロッキング受付             */
#define E_BOVR      -58         /* 0xFFC6: バッファオーバフロー             */


/***********************************
        機能コード
 ***********************************/
#include "uC3fncode.h"


/*******************************************
        トレース機能パラメータ
 *******************************************/

EXTERN  void _kernel_cside_tarce_1(void);
EXTERN  void _kernel_cside_tarce_2(void);
EXTERN  void _kernel_cside_tarce_3(void);
EXTERN  void _kernel_cside_tarce_4(void);
EXTERN  void _kernel_cside_tarce_5(void);
EXTERN  void _kernel_cside_tarce_6(void);
EXTERN  void _kernel_advice_tarce_1(void);
EXTERN  void _kernel_advice_tarce_2(void);
EXTERN  void _kernel_advice_tarce_3(void);
EXTERN  void _kernel_cubegear_task_trace(void);

#define TRACE_DISABLE               (FP)0

#define CSIDE_TASK_TRACE            (FP)_kernel_cside_tarce_1
#define CSIDE_SYSCALL_TRACE         (FP)_kernel_cside_tarce_2
#define CSIDE_TASK_SYSCALL_TRACE    (FP)_kernel_cside_tarce_3
#define CSIDE_TASK_INT_TRACE        (FP)_kernel_cside_tarce_4
#define CSIDE_FULL_TRACE            (FP)_kernel_cside_tarce_5
#define CSIDE_MEMBLK_TRACE          (FP)_kernel_cside_tarce_6
#define ADVICE_TASK_TRACE           (FP)_kernel_advice_tarce_1
#define ADVICE_TASK_SYSCALL_TRACE   (FP)_kernel_advice_tarce_2
#define CubeGEAR_TASK_TRACE         (FP)_kernel_cubegear_task_trace

/*******************************************
        エージェント機能パラメータ
 *******************************************/

EXTERN  void _kernel_cside_agent(void);

#define AGENT_DISABLE               (FP)0

#define CSIDE_AGENT                 (FP)_kernel_cside_agent


/*******************************************
        コンフィグレーションパラメータ
 *******************************************/

EXTERN  void _kernel_zero_init_stack(INT *, SIZE, ID);
EXTERN  void _kernel_id_init_stack(INT *, SIZE, ID);

#define SYSTEM_IDLE             (FP)0
#define USER_IDLE(p1)           (FP)(p1)

#define STACK_NO_INIT           (FP)0
#define STACK_ZERO_INIT         (FP)&_kernel_zero_init_stack
#define STACK_ID_INIT           (FP)&_kernel_id_init_stack
#define STACK_USER_INIT(p1)     ((FP)&(p1))

/***************************************
        システム初期化スタート関数
 ***************************************/

EXTERN  ER      start_uC3(T_CSYS *sys, FP inihdr);

/***********************************
        再定義システムコール
 ***********************************/

EXTERN  ER      iact_tsk(ID tskid);
EXTERN  ER      slp_tsk(void);
EXTERN  ER      iwup_tsk(ID tskid);
EXTERN  ER      irel_wai(ID tskid);
EXTERN  ER      isig_sem(ID semid);
EXTERN  ER      wai_sem(ID semid);
EXTERN  ER      pol_sem(ID semid);
EXTERN  ER      iset_flg(ID flgid, FLGPTN setptn);
EXTERN  ER      wai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn);
EXTERN  ER      pol_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn);
EXTERN  ER      ipsnd_dtq(ID dtqid, VP_INT data);
EXTERN  ER      psnd_dtq(ID dtqid, VP_INT data);
EXTERN  ER      snd_dtq(ID dtqid, VP_INT data);
EXTERN  ER      ifsnd_dtq(ID dtqid, VP_INT data);
EXTERN  ER      prcv_dtq(ID dtqid, VP_INT *p_data);
EXTERN  ER      rcv_dtq(ID dtqid, VP_INT *p_data);
EXTERN  ER      rcv_mbx(ID mbxid, T_MSG **ppk_msg);
EXTERN  ER      prcv_mbx(ID mbxid, T_MSG **ppk_msg);
EXTERN  ER      loc_mtx(ID mtxid);
EXTERN  ER      ploc_mtx(ID mtxid);
EXTERN  ER      snd_mbf(ID mbfid, VP msg, UINT msgsz);
EXTERN  ER      psnd_mbf(ID mbfid, VP msg, UINT msgsz);
EXTERN  ER_UINT rcv_mbf(ID mbfid, VP msg);
EXTERN  ER_UINT prcv_mbf(ID mbfid, VP msg);
EXTERN  ER_UINT cal_por(ID porid, RDVPTN calptn, VP msg, UINT cmsgsz);
EXTERN  ER_UINT acp_por(ID porid, RDVPTN acpptn, RDVNO *p_rdvno, VP msg);
EXTERN  ER_UINT pacp_por(ID porid, RDVPTN acpptn, RDVNO *p_rdvno, VP msg);
EXTERN  ER      get_mpf(ID mpfid, VP *p_blk);
EXTERN  ER      pget_mpf(ID mpfid, VP *p_blk);
EXTERN  ER      get_mpl(ID mplid, UINT blksz, VP *p_blk);
EXTERN  ER      pget_mpl(ID mplid, UINT blksz, VP *p_blk);
EXTERN  ER      irot_rdq(PRI tskpri);
EXTERN  ER      iget_tid(ID *p_tskid);
EXTERN  ER      iloc_cpu(void);
EXTERN  ER      iunl_cpu(void);

EXTERN  ER      ivact_tsk(ID coreid, ID tskid);
EXTERN  ER      ivwup_tsk(ID coreid, ID tskid);
EXTERN  ER      ivrel_wai(ID coreid, ID tskid);
EXTERN  ER      ivsig_sem(ID coreid, ID semid);
EXTERN  ER      ivset_flg(ID coreid, ID flgid, FLGPTN setptn);
EXTERN  ER      ivpsnd_dtq(ID coreid, ID dtqid, VP_INT data);
EXTERN  ER      ivfsnd_dtq(ID coreid, ID dtqid, VP_INT data);
EXTERN  ER      ivrot_rdq(ID coreid, PRI tskpri);
EXTERN  ER      ivsig_tim(ID coreid);

#define iact_tsk(p1)            (act_tsk(p1))
#define slp_tsk()               (tslp_tsk(TMO_FEVR))
#define iwup_tsk(p1)            (wup_tsk(p1))
#define irel_wai(p1)            (rel_wai(p1))
#define isig_sem(p1)            (sig_sem(p1))
#define pol_sem(p1)             (twai_sem((p1),TMO_POL))
#define wai_sem(p1)             (twai_sem((p1),TMO_FEVR))
#define iset_flg(p1,p2)         (set_flg((p1),(p2)))
#define pol_flg(p1,p2,p3,p4)    (twai_flg((p1),(p2),(p3),(p4),TMO_POL))
#define wai_flg(p1,p2,p3,p4)    (twai_flg((p1),(p2),(p3),(p4),TMO_FEVR))
#define ipsnd_dtq(p1,p2)        (tsnd_dtq((p1),(p2),TMO_POL))
#define psnd_dtq(p1,p2)         (tsnd_dtq((p1),(p2),TMO_POL))
#define snd_dtq(p1,p2)          (tsnd_dtq((p1),(p2),TMO_FEVR))
#define ifsnd_dtq(p1,p2)        (fsnd_dtq((p1),(p2)))
#define prcv_dtq(p1,p2)         (trcv_dtq((p1),(p2),TMO_POL))
#define rcv_dtq(p1,p2)          (trcv_dtq((p1),(p2),TMO_FEVR))
#define prcv_mbx(p1,p2)         (trcv_mbx((p1),(p2),TMO_POL))
#define rcv_mbx(p1,p2)          (trcv_mbx((p1),(p2),TMO_FEVR))
#define ploc_mtx(p1)            (tloc_mtx((p1),TMO_POL))
#define loc_mtx(p1)             (tloc_mtx((p1),TMO_FEVR))
#define psnd_mbf(p1,p2,p3)      (tsnd_mbf((p1),(p2),(p3),TMO_POL))
#define snd_mbf(p1,p2,p3)       (tsnd_mbf((p1),(p2),(p3),TMO_FEVR))
#define prcv_mbf(p1,p2)         (trcv_mbf((p1),(p2),TMO_POL))
#define rcv_mbf(p1,p2)          (trcv_mbf((p1),(p2),TMO_FEVR))
#define cal_por(p1,p2,p3,p4)    (tcal_por((p1),(p2),(p3),(p4),TMO_FEVR))
#define pacp_por(p1,p2,p3,p4)   (tacp_por((p1),(p2),(p3),(p4),TMO_POL))
#define acp_por(p1,p2,p3,p4)    (tacp_por((p1),(p2),(p3),(p4),TMO_FEVR))
#define pget_mpf(p1,p2)         (tget_mpf((p1),(p2),TMO_POL))
#define get_mpf(p1,p2)          (tget_mpf((p1),(p2),TMO_FEVR))
#define pget_mpl(p1,p2,p3)      (tget_mpl((p1),(p2),(p3),TMO_POL))
#define get_mpl(p1,p2,p3)       (tget_mpl((p1),(p2),(p3),TMO_FEVR))
#define irot_rdq(p1)            (rot_rdq(p1))
#define iget_tid(p1)            (get_tid(p1))
#define iloc_cpu()              (loc_cpu())
#define iunl_cpu()              (unl_cpu())

#define ivact_tsk(p1,p2)        (vact_tsk((p1),(p2)))
#define ivwup_tsk(p1,p2)        (vwup_tsk((p1),(p2)))
#define ivrel_wai(p1,p2)        (vrel_wai((p1),(p2)))
#define ivsig_sem(p1,p2)        (vsig_sem((p1),(p2)))
#define ivset_flg(p1,p2,p3)     (vset_flg((p1),(p2),(p3)))
#define ivpsnd_dtq(p1,p2,p3)    (vpsnd_dtq((p1),(p2),(p3)))
#define ivfsnd_dtq(p1,p2,p3)    (vfsnd_dtq((p1),(p2),(p3)))
#define ivrot_rdq(p1,p2)        (vrot_rdq((p1),(p2)))


/***************************************
        システムコールフック関数
 ***************************************/

#ifndef DISABLE_HOOK
#ifndef _UC3SYS_H_
#define acre_tsk                    _kernel_acre_tsk
#define cre_tsk                     _kernel_cre_tsk
#define del_tsk                     _kernel_del_tsk
#define act_tsk                     _kernel_act_tsk
#define can_act                     _kernel_can_act
#define sta_tsk                     _kernel_sta_tsk
#define ext_tsk                     _kernel_ext_tsk
#define exd_tsk                     _kernel_exd_tsk
#define ter_tsk                     _kernel_ter_tsk
#define chg_pri                     _kernel_chg_pri
#define get_pri                     _kernel_get_pri
#define ref_tsk                     _kernel_ref_tsk
#define ref_tst                     _kernel_ref_tst
#define tslp_tsk                    _kernel_tslp_tsk
#define wup_tsk                     _kernel_wup_tsk
#define can_wup                     _kernel_can_wup
#define rel_wai                     _kernel_rel_wai
#define sus_tsk                     _kernel_sus_tsk
#define rsm_tsk                     _kernel_rsm_tsk
#define frsm_tsk                    _kernel_frsm_tsk
#define dly_tsk                     _kernel_dly_tsk
#define acre_sem                    _kernel_acre_sem
#define cre_sem                     _kernel_cre_sem
#define del_sem                     _kernel_del_sem
#define sig_sem                     _kernel_sig_sem
#define twai_sem                    _kernel_twai_sem
#define ref_sem                     _kernel_ref_sem
#define acre_flg                    _kernel_acre_flg
#define cre_flg                     _kernel_cre_flg
#define del_flg                     _kernel_del_flg
#define set_flg                     _kernel_set_flg
#define clr_flg                     _kernel_clr_flg
#define twai_flg                    _kernel_twai_flg
#define ref_flg                     _kernel_ref_flg
#define acre_dtq                    _kernel_acre_dtq
#define cre_dtq                     _kernel_cre_dtq
#define del_dtq                     _kernel_del_dtq
#define tsnd_dtq                    _kernel_tsnd_dtq
#define fsnd_dtq                    _kernel_fsnd_dtq
#define trcv_dtq                    _kernel_trcv_dtq
#define ref_dtq                     _kernel_ref_dtq
#define acre_mbx                    _kernel_acre_mbx
#define cre_mbx                     _kernel_cre_mbx
#define del_mbx                     _kernel_del_mbx
#define snd_mbx                     _kernel_snd_mbx
#define trcv_mbx                    _kernel_trcv_mbx
#define ref_mbx                     _kernel_ref_mbx
#define acre_mtx                    _kernel_acre_mtx
#define cre_mtx                     _kernel_cre_mtx
#define del_mtx                     _kernel_del_mtx
#define unl_mtx                     _kernel_unl_mtx
#define tloc_mtx                    _kernel_tloc_mtx
#define ref_mtx                     _kernel_ref_mtx
#define acre_mbf                    _kernel_acre_mbf
#define cre_mbf                     _kernel_cre_mbf
#define del_mbf                     _kernel_del_mbf
#define tsnd_mbf                    _kernel_tsnd_mbf
#define trcv_mbf                    _kernel_trcv_mbf
#define ref_mbf                     _kernel_ref_mbf
#define acre_por                    _kernel_acre_por
#define cre_por                     _kernel_cre_por
#define del_por                     _kernel_del_por
#define tcal_por                    _kernel_tcal_por
#define tacp_por                    _kernel_tacp_por
#define fwd_por                     _kernel_fwd_por
#define rpl_rdv                     _kernel_rpl_rdv
#define ref_por                     _kernel_ref_por
#define ref_rdv                     _kernel_ref_rdv
#define acre_mpf                    _kernel_acre_mpf
#define cre_mpf                     _kernel_cre_mpf
#define del_mpf                     _kernel_del_mpf
#define tget_mpf                    _kernel_tget_mpf
#define rel_mpf                     _kernel_rel_mpf
#define ref_mpf                     _kernel_ref_mpf
#define acre_mpl                    _kernel_acre_mpl
#define cre_mpl                     _kernel_cre_mpl
#define del_mpl                     _kernel_del_mpl
#define tget_mpl                    _kernel_tget_mpl
#define rel_mpl                     _kernel_rel_mpl
#define ref_mpl                     _kernel_ref_mpl
#define set_tim                     _kernel_set_tim
#define get_tim                     _kernel_get_tim
#define acre_cyc                    _kernel_acre_cyc
#define cre_cyc                     _kernel_cre_cyc
#define del_cyc                     _kernel_del_cyc
#define sta_cyc                     _kernel_sta_cyc
#define stp_cyc                     _kernel_stp_cyc
#define ref_cyc                     _kernel_ref_cyc
#define acre_alm                    _kernel_acre_alm
#define cre_alm                     _kernel_cre_alm
#define del_alm                     _kernel_del_alm
#define sta_alm                     _kernel_sta_alm
#define stp_alm                     _kernel_stp_alm
#define ref_alm                     _kernel_ref_alm
#define def_ovr                     _kernel_def_ovr
#define sta_ovr                     _kernel_sta_ovr
#define stp_ovr                     _kernel_stp_ovr
#define ref_ovr                     _kernel_ref_ovr
#define rot_rdq                     _kernel_rot_rdq
#define get_tid                     _kernel_get_tid
#define loc_cpu                     _kernel_loc_cpu
#define unl_cpu                     _kernel_unl_cpu
#define dis_dsp                     _kernel_dis_dsp
#define ena_dsp                     _kernel_ena_dsp
#define sns_ctx                     _kernel_sns_ctx
#define sns_loc                     _kernel_sns_loc
#define sns_dsp                     _kernel_sns_dsp
#define sns_dpn                     _kernel_sns_dpn
#define ref_sys                     _kernel_ref_sys
#define def_inh                     _kernel_def_inh
#define acre_isr                    _kernel_acre_isr
#define cre_isr                     _kernel_cre_isr
#define del_isr                     _kernel_del_isr
#define ref_isr                     _kernel_ref_isr
#define chg_ims                     _kernel_chg_ims
#define get_ims                     _kernel_get_ims
#define ena_int                     _kernel_ena_int
#define dis_int                     _kernel_dis_int
#define ref_cfg                     _kernel_ref_cfg
#define ref_ver                     _kernel_ref_ver
#define vact_tsk                    _kernel_vact_tsk
#define vsta_tsk                    _kernel_vsta_tsk
#define vwup_tsk                    _kernel_vwup_tsk
#define vrel_wai                    _kernel_vrel_wai
#define vsig_sem                    _kernel_vsig_sem
#define vpol_sem                    _kernel_vpol_sem
#define vset_flg                    _kernel_vset_flg
#define vclr_flg                    _kernel_vclr_flg
#define vpol_flg                    _kernel_vpol_flg
#define vpsnd_dtq                   _kernel_vpsnd_dtq
#define vfsnd_dtq                   _kernel_vfsnd_dtq
#define vprcv_dtq                   _kernel_vprcv_dtq
#define vrot_rdq                    _kernel_vrot_rdq

#endif
#endif

/* タスク管理機能 */

EXTERN  ER_ID   acre_tsk(T_CTSK *pk_ctsk);
EXTERN  ER      cre_tsk(ID tskid, T_CTSK *pk_ctsk);
EXTERN  ER      del_tsk(ID tskid);
EXTERN  ER      act_tsk(ID tskid);
EXTERN  ER_UINT can_act(ID tskid);
EXTERN  ER      sta_tsk(ID tskid, VP_INT stacd);
EXTERN  void    ext_tsk(void);
EXTERN  void    exd_tsk(void);
EXTERN  ER      ter_tsk(ID tskid);
EXTERN  ER      chg_pri(ID tskid, PRI tskpri);
EXTERN  ER      get_pri(ID tskid, PRI *p_tskpri);
EXTERN  ER      ref_tsk(ID tskid, T_RTSK *pk_rtsk);
EXTERN  ER      ref_tst(ID tskid, T_RTST *pk_rtst);

/* タスク付属同期機能 */

EXTERN  ER      tslp_tsk(TMO timout);
EXTERN  ER      wup_tsk(ID tskid);
EXTERN  ER_UINT can_wup(ID tskid);
EXTERN  ER      rel_wai(ID tskid);
EXTERN  ER      sus_tsk(ID tskid);
EXTERN  ER      rsm_tsk(ID tskid);
EXTERN  ER      frsm_tsk(ID tskid);
EXTERN  ER      dly_tsk(RELTIM dlytim);

/* タスク例外処理機能 */

EXTERN  ER      def_tex(ID tskid, T_DTEX *pk_dtex);
EXTERN  ER      ras_tex(ID tskid, TEXPTN rasptn);
EXTERN  ER      iras_tex(ID tskid, TEXPTN rasptn);
EXTERN  ER      dis_tex(void);
EXTERN  ER      ena_tex(void);
EXTERN  BOOL    sns_tex(void);
EXTERN  ER      ref_tex(ID tskid, T_RTEX *pk_rtex);

/* 同期・通信機能（セマフォ） */

EXTERN  ER_ID   acre_sem(T_CSEM *pk_csem);
EXTERN  ER      cre_sem(ID semid, T_CSEM *pk_csem);
EXTERN  ER      del_sem(ID semid);
EXTERN  ER      sig_sem(ID semid);
EXTERN  ER      twai_sem(ID semid, TMO tmout);
EXTERN  ER      ref_sem(ID semid, T_RSEM *pk_rsem);

/* 同期・通信機能（イベントフラグ） */

EXTERN  ER_ID   acre_flg(T_CFLG *pk_cflg);
EXTERN  ER      cre_flg(ID flgid, T_CFLG *pk_cflg);
EXTERN  ER      del_flg(ID flgid);
EXTERN  ER      set_flg(ID flgid, FLGPTN setptn);
EXTERN  ER      clr_flg(ID flgid, FLGPTN clrptn);
EXTERN  ER      twai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn, TMO tmout);
EXTERN  ER      ref_flg(ID flgid, T_RFLG *pk_rflg);

/* 同期・通信機能（データキュー） */

EXTERN  ER_ID   acre_dtq(T_CDTQ *pk_cdtq);
EXTERN  ER      cre_dtq(ID dtqid, T_CDTQ *pk_cdtq);
EXTERN  ER      del_dtq(ID dtqid);
EXTERN  ER      tsnd_dtq(ID dtqid, VP_INT data, TMO tmout);
EXTERN  ER      fsnd_dtq(ID dtqid, VP_INT data);
EXTERN  ER      trcv_dtq(ID dtqid, VP_INT *p_data, TMO tmout);
EXTERN  ER      ref_dtq(ID dtqid, T_RDTQ *pk_rdtq);

/* 同期・通信機能（メールボックス） */

EXTERN  ER_ID   acre_mbx(T_CMBX *pk_cmbx);
EXTERN  ER      cre_mbx(ID mbxid, T_CMBX *pk_cmbx);
EXTERN  ER      del_mbx(ID mbxid);
EXTERN  ER      snd_mbx(ID mbxid, T_MSG *pk_msg);
EXTERN  ER      trcv_mbx(ID mbxid, T_MSG **ppk_msg, TMO tmout);
EXTERN  ER      ref_mbx(ID mbxid, T_RMBX *pk_rmbx);

/* 拡張同期・通信機能（ミューテックス） */

EXTERN  ER_ID   acre_mtx(T_CMTX *pk_cmtx);
EXTERN  ER      cre_mtx(ID mtxid, T_CMTX *pk_cmtx);
EXTERN  ER      del_mtx(ID mtxid);
EXTERN  ER      unl_mtx(ID mtxid);
EXTERN  ER      tloc_mtx(ID mtxid, TMO tmout);
EXTERN  ER      ref_mtx(ID mtxid, T_RMTX *pk_rmtx);

/* 拡張同期・通信機能（メッセージバッファ） */

EXTERN  ER_ID   acre_mbf(T_CMBF *pk_cmbf);
EXTERN  ER      cre_mbf(ID mbfid, T_CMBF *pk_cmbf);
EXTERN  ER      del_mbf(ID mbfid);
EXTERN  ER      tsnd_mbf(ID mbfid, VP msg, UINT msgsz, TMO tmout);
EXTERN  ER_UINT trcv_mbf(ID mbfid, VP msg, TMO tmout);
EXTERN  ER      ref_mbf(ID mbfid, T_RMBF *pk_rmbf);

/* 拡張同期・通信機能（ランデブ） */

EXTERN  ER_ID   acre_por(T_CPOR *pk_cpor);
EXTERN  ER      cre_por(ID porid, T_CPOR *pk_cpor);
EXTERN  ER      del_por(ID porid);
EXTERN  ER_UINT tcal_por(ID porid, RDVPTN calptn, VP msg, UINT cmsgsz, TMO tmout);
EXTERN  ER_UINT tacp_por(ID porid, RDVPTN acpptn, RDVNO *p_rdvno, VP msg, TMO tmout);
EXTERN  ER      fwd_por(ID porid, RDVPTN calptn, RDVNO rdvno, VP msg, UINT cmsgsz);
EXTERN  ER      rpl_rdv(RDVNO rdvno, VP msg, UINT msgsz);
EXTERN  ER      ref_por(ID porid, T_RPOR *pk_rpor);
EXTERN  ER      ref_rdv(RDVNO rdvno, T_RRDV *pk_rrdv);

/* メモリプール管理機能（固定長メモリプール） */

EXTERN  ER_ID   acre_mpf(T_CMPF *pk_cmpf);
EXTERN  ER      cre_mpf(ID mpfid, T_CMPF *pk_cmpf);
EXTERN  ER      del_mpf(ID mpfid);
EXTERN  ER      tget_mpf(ID mpfid, VP *p_blk, TMO tmout);
EXTERN  ER      rel_mpf(ID mpfid, VP p_blk);
EXTERN  ER      ref_mpf(ID mpfid, T_RMPF *pk_rmpf);

/* メモリプール管理機能（可変長メモリプール） */

EXTERN  ER_ID   acre_mpl(T_CMPL *pk_cmpl);
EXTERN  ER      cre_mpl(ID mplid, T_CMPL *pk_cmpl);
EXTERN  ER      del_mpl(ID mplid);
EXTERN  ER      tget_mpl(ID mplid, UINT blksz, VP *p_blk, TMO tmout);
EXTERN  ER      rel_mpl(ID mplid, VP p_blk);
EXTERN  ER      ref_mpl(ID mplid, T_RMPL *pk_rmpl);

/* 時間管理機能（システム時刻管理） */

EXTERN  ER      set_tim(SYSTIM *p_systim);
EXTERN  ER      get_tim(SYSTIM *p_systim);
EXTERN  ER      isig_tim(void);
EXTERN  UW      vget_tms(void);

/* 時間管理機能（周期ハンドラ） */

EXTERN  ER_ID   acre_cyc(T_CCYC *pk_ccyc);
EXTERN  ER      cre_cyc(ID cycid, T_CCYC *pk_ccyc);
EXTERN  ER      del_cyc(ID cycid);
EXTERN  ER      sta_cyc(ID cycid);
EXTERN  ER      stp_cyc(ID cycid);
EXTERN  ER      ref_cyc(ID cycid, T_RCYC *pk_rcyc);

/* 時間管理機能（アラームハンドラ） */

EXTERN  ER_ID   acre_alm(T_CALM *pk_calm);
EXTERN  ER      cre_alm(ID almid, T_CALM *pk_calm);
EXTERN  ER      del_alm(ID almid);
EXTERN  ER      sta_alm(ID almid, RELTIM almtim);
EXTERN  ER      stp_alm(ID almid);
EXTERN  ER      ref_alm(ID almid, T_RALM *pk_ralm);

/* 時間管理機能（オーバランハンドラ） */

EXTERN  ER      def_ovr(T_DOVR *pk_dovr);
EXTERN  ER      ivsig_ovr(void);
EXTERN  ER      sta_ovr(ID tskid, OVRTIM ovrtim);
EXTERN  ER      stp_ovr(ID tskid);
EXTERN  ER      ref_ovr(ID tskid, T_ROVR *pk_rovr);

/* システム状態管理機能 */

EXTERN  ER      rot_rdq(PRI tskpri);
EXTERN  ER      get_tid(ID *p_tskid);
EXTERN  ER      loc_cpu(void);
EXTERN  ER      unl_cpu(void);
EXTERN  ER      dis_dsp(void);
EXTERN  ER      ena_dsp(void);
EXTERN  BOOL    sns_ctx(void);
EXTERN  BOOL    sns_loc(void);
EXTERN  BOOL    sns_dsp(void);
EXTERN  BOOL    sns_dpn(void);
EXTERN  ER      ref_sys(T_RSYS *pk_rsys);

/* 割込み管理機能 */

EXTERN  ER      def_inh(INHNO inhno, T_DINH *pk_dinh);
EXTERN  ER_ID   acre_isr(T_CISR *);
EXTERN  ER      cre_isr(ID isrid, T_CISR *pk_cisr);
EXTERN  ER      del_isr(ID isrid);
EXTERN  ER      ref_isr(ID isrid, T_RISR *pk_risr);
EXTERN  ER      chg_ims(IMASK imask);
EXTERN  ER      get_ims(IMASK *p_imask);
EXTERN  ER      ena_int(INTNO intno);
EXTERN  ER      dis_int(INTNO intno);

/* デバイス管理機能 */

EXTERN  ER      vdef_dev(ID devid, T_CDEV *pk_cdev);
EXTERN  ER      vctr_dev(ID devid, ID funcid, VP ctrdev);

/* システム構成管理機能 */

EXTERN  ER      def_exc(EXCNO excno, T_DEXC *pk_dexc);
EXTERN  ER      vdef_err(ATR atr, FP func);
EXTERN  ER      ref_cfg(T_RCFG *pk_rcfg);
EXTERN  ER      ref_ver(T_RVER *pk_rver);

/* マルチコア拡張管理機能 */

EXTERN  ER      ivsig_tim(ID coreid);
EXTERN  ER      vact_tsk(ID coreid, ID tskid);
EXTERN  ER      vsta_tsk(ID coreid, ID tskid, VP_INT stacd);
EXTERN  ER      vwup_tsk(ID coreid, ID tskid);
EXTERN  ER      vrel_wai(ID coreid, ID tskid);
EXTERN  ER      vsig_sem(ID coreid, ID semid);
EXTERN  ER      vpol_sem(ID coreid, ID semid);
EXTERN  ER      vset_flg(ID coreid, ID flgid, FLGPTN setptn);
EXTERN  ER      vclr_flg(ID coreid, ID flgid, FLGPTN clrptn);
EXTERN  ER      vpol_flg(ID coreid, ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn);
EXTERN  ER      vpsnd_dtq(ID coreid, ID dtqid, VP_INT data);
EXTERN  ER      vfsnd_dtq(ID coreid, ID dtqid, VP_INT data);
EXTERN  ER      vprcv_dtq(ID coreid, ID dtqid, VP_INT *p_data);
EXTERN  ER      vrot_rdq(ID coreid, PRI tskpri);

#ifdef __cplusplus
}
#endif

#endif  /* _KERNEL_H_ */
