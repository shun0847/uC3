/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ff.h"
#include "diskio.h"
#include "sdmmc_config.h"
#include "sample_fatfs_cfg.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* buffer size (in byte) for read/write operations */
#define BUFFER_SIZE (512U)
#define CLI_LINE_MAX (128U)
#define CLI_PATH_MAX (256U)
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static int fatfs_task(VP_INT exinf);
static void cli_print_help(void);
static void cli_ls(const char *path);
static void cli_touch(const char *path);
static void cli_mkdir(const char *path);
static void cli_cat(const char *path);
static void cli_write(const char *path, const char *text, bool append);
static void cli_rm(const char *path);
static void cli_cd(const char *path);
static void cli_prompt_loop(void);
static const char *cli_cwd_get(void);
static void cli_build_path(char *out, size_t out_size, const char *path);
static void cli_normalize_path(char *path);
static void cli_redraw_line(const char *prompt, const char *buf, UINT len);
static void selftest(void);
static void mtx_test(void);
static void mtx_test_task(VP_INT exinf);
static void mmc_print_bus_status(void);
/*******************************************************************************
 * Variables
 ******************************************************************************/
static FATFS g_fileSystem; /* File system object */
extern mmc_card_t g_mmc;
volatile uint32_t g_usdhc3_irq_count = 0U;
static char g_cli_cwd[CLI_PATH_MAX] = "/";
static const char g_mtx_test_path[] = "0:/mtx_test.log";
static bool g_mtx_test_event_ready = false;
static OSA_EVENT_HANDLE_DEFINE(g_mtx_test_event);
/* @brief decription about the read/write buffer
 * The size of the read/write buffer in this driver example is multiple of 512, since DDR mode support 512-byte
 * block size only and our middleware switch the timing mode automatically per device capability. You can define the
 * buffer size to meet your requirement.If the card support partial access, you can also re-define the block size.
 * The address of the read/write buffer should align to the specific DMA data buffer address align value if
 * DMA transfer is used, otherwise the buffer address is not important.
 * At the same time buffer address/size should be aligned to the cache line size if cache is support.
 */
/*! @brief Data written to the card */
SDK_ALIGN(uint8_t g_bufferWrite[BUFFER_SIZE], BOARD_SDMMC_DATA_BUFFER_ALIGN_SIZE);
/*! @brief Data read from the card */
SDK_ALIGN(uint8_t g_bufferRead[BUFFER_SIZE], BOARD_SDMMC_DATA_BUFFER_ALIGN_SIZE);

/*******************************************************************************
 * Code
 ******************************************************************************/
static const char *mmc_bus_timing_name(uint32_t timing)
{
    switch (timing)
    {
        case kMMC_HighSpeedTimingNone:
            return "Legacy";
        case kMMC_HighSpeedTiming:
            return "HighSpeed";
        case kMMC_HighSpeed200Timing:
            return "HS200";
        case kMMC_HighSpeed400Timing:
            return "HS400";
        case kMMC_EnhanceHighSpeed400Timing:
            return "HS400ES";
        default:
            return "Unknown";
    }
}

static const char *mmc_bus_width_name(uint32_t width)
{
    switch (width)
    {
        case kMMC_DataBusWidth1bit:
            return "1-bit";
        case kMMC_DataBusWidth4bit:
            return "4-bit";
        case kMMC_DataBusWidth8bit:
            return "8-bit";
        case kMMC_DataBusWidth4bitDDR:
            return "4-bit DDR";
        case kMMC_DataBusWidth8bitDDR:
            return "8-bit DDR";
        default:
            return "Unknown";
    }
}

static void mmc_print_bus_status(void)
{
    PRINTF("MMC bus timing: %s\r\n", mmc_bus_timing_name(g_mmc.busTiming));
    PRINTF("MMC bus width : %s\r\n", mmc_bus_width_name(g_mmc.busWidth));
    PRINTF("MMC bus clock : %u Hz\r\n", (unsigned)g_mmc.busClock_Hz);
    PRINTF("MMC VCCQ      : %u\r\n", (unsigned)g_mmc.hostVoltageWindowVCCQ);
    PRINTF("MMC cardType  : 0x%02x\r\n", (unsigned)g_mmc.extendedCsd.cardType);
}
static void cli_print_help(void)
{
    PRINTF("\r\nCommands:\r\n");
    PRINTF("  help                  - show this help\r\n");
    PRINTF("  ls [path]             - list directory (default .)\r\n");
    PRINTF("  cd [path]             - change directory\r\n");
    PRINTF("  mkdir [path]           - create directory\r\n");
    PRINTF("  touch [path]          - create empty file\r\n");
    PRINTF("  cat [path]            - print file contents\r\n");
    PRINTF("  echo \"text\" > [path]  - write text to file (overwrite)\r\n");
    PRINTF("  echo \"text\" >> [path] - append text to file\r\n");
    PRINTF("  rm [path]             - delete file or empty directory\r\n");
    PRINTF("  selftest              - run OSA/FATFS quick test\r\n");
    PRINTF("  mtx_test              - run FATFS multi-task mutex test\r\n");
    PRINTF("  exit                  - stop CLI loop\r\n\r\n");
}

static const char *cli_cwd_get(void)
{
    return (g_cli_cwd[0] == '\0') ? "/" : g_cli_cwd;
}

static void cli_build_path(char *out, size_t out_size, const char *path)
{
    const char *cwd = cli_cwd_get();
    if (path == NULL || path[0] == '\0')
    {
        (void)snprintf(out, out_size, "%s", cwd);
        cli_normalize_path(out);
        return;
    }
    if (path[0] == '/')
    {
        (void)snprintf(out, out_size, "%s", path);
        cli_normalize_path(out);
        return;
    }
    if (strcmp(cwd, "/") == 0)
    {
        (void)snprintf(out, out_size, "/%s", path);
    }
    else
    {
        (void)snprintf(out, out_size, "%s/%s", cwd, path);
    }
    cli_normalize_path(out);
}

static void cli_normalize_path(char *path)
{
    char tmp[CLI_PATH_MAX];
    size_t out = 0U;
    size_t i = 0U;

    if (path == NULL || path[0] == '\0')
    {
        (void)snprintf(path, CLI_PATH_MAX, "/");
        return;
    }
    if (path[0] != '/')
    {
        /* Expect absolute path at this point. */
        (void)snprintf(tmp, sizeof(tmp), "/%s", path);
        (void)snprintf(path, CLI_PATH_MAX, "%s", tmp);
    }

    /* Build normalized path in tmp. */
    tmp[out++] = '/';
    while (path[i] != '\0')
    {
        while (path[i] == '/')
        {
            i++;
        }
        if (path[i] == '\0')
        {
            break;
        }
        /* Extract next segment. */
        size_t seg_start = i;
        while (path[i] != '/' && path[i] != '\0')
        {
            i++;
        }
        size_t seg_len = i - seg_start;
        if (seg_len == 1 && path[seg_start] == '.')
        {
            continue;
        }
        if (seg_len == 2 && path[seg_start] == '.' && path[seg_start + 1] == '.')
        {
            if (out > 1U)
            {
                out--;
                while (out > 0U && tmp[out - 1U] != '/')
                {
                    out--;
                }
            }
            continue;
        }
        if (out > 1U)
        {
            tmp[out++] = '/';
        }
        if (out + seg_len >= sizeof(tmp))
        {
            break;
        }
        (void)memcpy(&tmp[out], &path[seg_start], seg_len);
        out += seg_len;
    }

    if (out == 0U)
    {
        tmp[out++] = '/';
    }
    tmp[out] = '\0';
    (void)snprintf(path, CLI_PATH_MAX, "%s", tmp);
}

static void cli_ls(const char *path)
{
    DIR directory;
    FILINFO info;
    FRESULT error;
    char fullpath[CLI_PATH_MAX];
    cli_build_path(fullpath, sizeof(fullpath), path);

    error = f_opendir(&directory, fullpath);
    if (error)
    {
        PRINTF("ls: cannot open %s (err=%d)\r\n", fullpath, error);
        return;
    }

    for (;;)
    {
        error = f_readdir(&directory, &info);
        if ((error != FR_OK) || (info.fname[0U] == 0U))
        {
            break;
        }
        if (info.fattrib & AM_DIR)
        {
            PRINTF(" %10lu %s <Dir>\r\n", (unsigned long)info.fsize, info.fname);
        }
        else
        {
            PRINTF(" %10lu %s\r\n", (unsigned long)info.fsize, info.fname);
        }
    }
}

static void cli_touch(const char *path)
{
    FIL file;
    FRESULT error;
    char fullpath[CLI_PATH_MAX];
    if (path == NULL || path[0] == '\0')
    {
        PRINTF("touch: missing path\r\n");
        return;
    }
    cli_build_path(fullpath, sizeof(fullpath), path);
    error = f_open(&file, fullpath, FA_CREATE_ALWAYS | FA_WRITE);
    if (error)
    {
        PRINTF("touch: failed (err=%d)\r\n", error);
        return;
    }
    (void)f_close(&file);
}

static void cli_mkdir(const char *path)
{
    FRESULT error;
    char fullpath[CLI_PATH_MAX];
    if (path == NULL || path[0] == '\0')
    {
        PRINTF("mkdir: missing path\r\n");
        return;
    }
    cli_build_path(fullpath, sizeof(fullpath), path);
    error = f_mkdir(fullpath);
    if (error && (error != FR_EXIST))
    {
        PRINTF("mkdir: failed (err=%d)\r\n", error);
    }
}

static void cli_cat(const char *path)
{
    FIL file;
    FRESULT error;
    UINT bytesRead;
    char fullpath[CLI_PATH_MAX];
    if (path == NULL || path[0] == '\0')
    {
        PRINTF("cat: missing path\r\n");
        return;
    }
    cli_build_path(fullpath, sizeof(fullpath), path);
    error = f_open(&file, fullpath, FA_READ);
    if (error)
    {
        PRINTF("cat: failed (err=%d)\r\n", error);
        return;
    }

    for (;;)
    {
        error = f_read(&file, g_bufferRead, sizeof(g_bufferRead), &bytesRead);
        if (error || bytesRead == 0U)
        {
            break;
        }
        for (UINT i = 0; i < bytesRead; i++)
        {
            PUTCHAR((char)g_bufferRead[i]);
        }
    }
    PUTCHAR('\r');
    PUTCHAR('\n');
    (void)f_close(&file);
}

static void cli_write(const char *path, const char *text, bool append)
{
    FIL file;
    FRESULT error;
    UINT bytesWritten;
    char fullpath[CLI_PATH_MAX];
    char local[CLI_LINE_MAX];
    if (path == NULL || path[0] == '\0' || text == NULL)
    {
        PRINTF("echo: usage echo \"text\" > [path]\r\n");
        return;
    }
    (void)snprintf(local, sizeof(local), "%s\r\n", text);
    cli_build_path(fullpath, sizeof(fullpath), path);
    if (append)
    {
        error = f_open(&file, fullpath, FA_OPEN_APPEND | FA_WRITE);
    }
    else
    {
        error = f_open(&file, fullpath, FA_CREATE_ALWAYS | FA_WRITE);
    }
    if (error)
    {
        PRINTF("echo: failed (err=%d)\r\n", error);
        return;
    }
    error = f_write(&file, local, (UINT)strlen(local), &bytesWritten);
    (void)f_close(&file);
    if (error || bytesWritten != (UINT)strlen(local))
    {
        PRINTF("echo: failed (err=%d)\r\n", error);
    }
}

static void cli_rm(const char *path)
{
    FRESULT error;
    char fullpath[CLI_PATH_MAX];
    if (path == NULL || path[0] == '\0')
    {
        PRINTF("rm: missing path\r\n");
        return;
    }
    cli_build_path(fullpath, sizeof(fullpath), path);
    error = f_unlink(fullpath);
    if (error)
    {
        PRINTF("rm: failed (err=%d)\r\n", error);
    }
}

static void cli_cd(const char *path)
{
    DIR directory;
    FRESULT error;
    char fullpath[CLI_PATH_MAX];
    if (path == NULL || path[0] == '\0')
    {
        PRINTF("cd: missing path\r\n");
        return;
    }
    cli_build_path(fullpath, sizeof(fullpath), path);
    error = f_opendir(&directory, fullpath);
    if (error)
    {
        PRINTF("cd: cannot open %s (err=%d)\r\n", fullpath, error);
        return;
    }
    (void)snprintf(g_cli_cwd, sizeof(g_cli_cwd), "%s", fullpath);
}

static void cli_redraw_line(const char *prompt, const char *buf, UINT len)
{
    PUTCHAR('\r');
    PRINTF("%s", prompt);
    for (UINT i = 0; i < len; i++)
    {
        PUTCHAR(buf[i]);
    }
}

static void cli_prompt_loop(void)
{
    char line[CLI_LINE_MAX];
    char line_copy[CLI_LINE_MAX];
    char prompt[CLI_PATH_MAX + 16U];
    UINT idx = 0;
    bool overflow = false;

    cli_print_help();
    for (;;)
    {
        (void)snprintf(prompt, sizeof(prompt), "fatfs:%s> ", cli_cwd_get());
        PRINTF("%s", prompt);
        idx = 0;
        overflow = false;
        for (;;)
        {
            char c = (char)GETCHAR();
            if (c == '\r' || c == '\n')
            {
                PUTCHAR('\r');
                PUTCHAR('\n');
                break;
            }
            /* Handle ESC sequences (e.g. Delete key: ESC [ 3 ~). */
            if ((unsigned char)c == 0x1B)
            {
                char c1 = (char)GETCHAR();
                if (c1 == '[')
                {
                    char c2 = (char)GETCHAR();
                    if (c2 == '3')
                    {
                        char c3 = (char)GETCHAR();
                        if (c3 == '~')
                        {
                            if (idx > 0U)
                            {
                                idx--;
                                PUTCHAR('\b');
                                PUTCHAR(' ');
                                PUTCHAR('\b');
                            }
                        }
                    }
                }
                continue;
            }
            if (overflow)
            {
                continue;
            }
            if (c == '\b' || c == 0x7f)
            {
                if (idx > 0U)
                {
                    idx--;
                    PUTCHAR('\b');
                    PUTCHAR(' ');
                    PUTCHAR('\b');
                }
                continue;
            }
            /* Avoid treating Ctrl+U (0x15) as line kill. */
            if ((unsigned char)c == 0x15)
            {
                cli_redraw_line(prompt, line, idx);
                continue;
            }
            //if (!isprint((unsigned char)c))
            if (((unsigned char)c < 0x20U) || ((unsigned char)c == 0x7FU)) 
            {
                continue;
            }
            if (idx < (CLI_LINE_MAX - 1U))
            {
                line[idx++] = c;
                PUTCHAR(c);
            }
            else
            {
                overflow = true;
            }
        }
        line[idx] = '\0';
        (void)snprintf(line_copy, sizeof(line_copy), "%s", line);
        if (overflow)
        {
            PRINTF("Input too long (max %u chars). Try again.\r\n", (unsigned)(CLI_LINE_MAX - 1U));
            continue;
        }

        char *cmd = line;
        while (*cmd && isspace((unsigned char)*cmd))
        {
            cmd++;
        }
        if (*cmd == '\0')
        {
            continue;
        }
        char *arg1 = cmd;
        while (*arg1 && !isspace((unsigned char)*arg1))
        {
            arg1++;
        }
        if (*arg1)
        {
            *arg1++ = '\0';
        }
        while (*arg1 && isspace((unsigned char)*arg1))
        {
            arg1++;
        }
        char *arg2 = arg1;
        while (*arg2 && !isspace((unsigned char)*arg2))
        {
            arg2++;
        }
        if (*arg2)
        {
            *arg2++ = '\0';
        }
        while (*arg2 && isspace((unsigned char)*arg2))
        {
            arg2++;
        }

        if (strcmp(cmd, "help") == 0)
        {
            cli_print_help();
        }
        else if (strcmp(cmd, "ls") == 0)
        {
            cli_ls(arg1);
        }
        else if (strcmp(cmd, "cd") == 0)
        {
            cli_cd(arg1);
        }
        else if (strcmp(cmd, "mkdir") == 0)
        {
            cli_mkdir(arg1);
        }
        else if (strcmp(cmd, "touch") == 0)
        {
            cli_touch(arg1);
        }
        else if (strcmp(cmd, "cat") == 0)
        {
            cli_cat(arg1);
        }
        else if ((strcmp(cmd, "echo") == 0) || (strcmp(cmd, "write") == 0))
        {
            char *p = line_copy;
            while (*p && isspace((unsigned char)*p))
            {
                p++;
            }
            if (strncmp(p, "echo", 4) == 0)
            {
                p += 4;
            }
            else if (strncmp(p, "write", 5) == 0)
            {
                p += 5;
            }
            while (*p && isspace((unsigned char)*p))
            {
                p++;
            }
            char *op = strstr(p, ">>");
            bool append = false;
            if (op != NULL)
            {
                append = true;
            }
            else
            {
                op = strchr(p, '>');
            }
            if (op == NULL)
            {
                PRINTF("echo: usage echo \"text\" > [path]\r\n");
                continue;
            }
            *op = '\0';
            op += append ? 2 : 1;
            while (*op && isspace((unsigned char)*op))
            {
                op++;
            }
            char *text = p;
            char *end = text + strlen(text);
            while ((end > text) && isspace((unsigned char)end[-1]))
            {
                end--;
            }
            *end = '\0';
            if ((text[0] == '\0') || (op[0] == '\0'))
            {
                PRINTF("echo: usage echo \"text\" > [path]\r\n");
                continue;
            }
            cli_write(op, text, append);
        }
        else if (strcmp(cmd, "rm") == 0)
        {
            cli_rm(arg1);
        }
        else if (strcmp(cmd, "selftest") == 0)
        {
            selftest();
        }
        else if (strcmp(cmd, "mtx_test") == 0)
        {
            mtx_test();
        }
        else if (strcmp(cmd, "exit") == 0)
        {
            break;
        }
        else
        {
            PRINTF("Unknown command: %s\r\n", cmd);
        }
    }
}

static void selftest(void)
{
    osa_status_t st;
    osa_event_flags_t flags = 0U;
    FRESULT f_err;
    FIL file;
    UINT bytesWritten;
    UINT bytesRead;
    uint32_t irq_before = 0U;
    uint32_t irq_after = 0U;
    char fullpath[CLI_PATH_MAX];

    PRINTF("\r\n[Self-test] start\r\n");

    /* Semaphore test */
    OSA_SEMAPHORE_HANDLE_DEFINE(semHandle);
    st = OSA_SemaphoreCreateBinary(semHandle);
    if (st != KOSA_StatusSuccess)
    {
        PRINTF("sem create: NG (%u)\r\n", (unsigned)st);
    }
    else
    {
        st = OSA_SemaphoreWait(semHandle, 10U);
        PRINTF("sem wait timeout: %s\r\n", (st == KOSA_StatusTimeout) ? "OK" : "NG");
        st = OSA_SemaphorePost(semHandle);
        PRINTF("sem post: %s\r\n", (st == KOSA_StatusSuccess) ? "OK" : "NG");
        st = OSA_SemaphoreWait(semHandle, 10U);
        PRINTF("sem wait success: %s\r\n", (st == KOSA_StatusSuccess) ? "OK" : "NG");
        (void)OSA_SemaphoreDestroy(semHandle);
    }

    /* Mutex test */
    OSA_MUTEX_HANDLE_DEFINE(mutexHandle);
    st = OSA_MutexCreate(mutexHandle);
    if (st != KOSA_StatusSuccess)
    {
        PRINTF("mutex create: NG (%u)\r\n", (unsigned)st);
    }
    else
    {
        OSA_MutexLock(mutexHandle, 10U);
        PRINTF("mutex lock: %s\r\n", (st == KOSA_StatusSuccess) ? "OK" : "NG");
        st = OSA_MutexUnlock(mutexHandle);
        PRINTF("mutex unlock: %s\r\n", (st == KOSA_StatusSuccess) ? "OK" : "NG");
        (void)OSA_MutexDestroy(mutexHandle);
    }

    /* Event test */
    OSA_EVENT_HANDLE_DEFINE(eventHandle);
    st = OSA_EventCreate(eventHandle, 1U);
    if (st != KOSA_StatusSuccess)
    {
        PRINTF("event create: NG (%u)\r\n", (unsigned)st);
    }
    else
    {
        st = OSA_EventWait(eventHandle, 0x1U, 1U, 10U, &flags);
        PRINTF("event wait timeout: %s\r\n", (st == KOSA_StatusTimeout) ? "OK" : "NG");
        st = OSA_EventSet(eventHandle, 0x1U);
        PRINTF("event set: %s\r\n", (st == KOSA_StatusSuccess) ? "OK" : "NG");
        st = OSA_EventWait(eventHandle, 0x1U, 1U, 10U, &flags);
        PRINTF("event wait success: %s\r\n", (st == KOSA_StatusSuccess) ? "OK" : "NG");
        (void)OSA_EventDestroy(eventHandle);
    }

    /* Time delay test */
    OSA_TimeDelay(1U);
    PRINTF("delay: done\r\n");

    /* FATFS + DMA/IRQ quick test */
    PRINTF("fatfs rw test: start\r\n");
    cli_build_path(fullpath, sizeof(fullpath), "selftest.bin");
    f_err = f_open(&file, fullpath, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
    if (f_err)
    {
        PRINTF("fatfs rw test: open NG (err=%d)\r\n", f_err);
        PRINTF("[Self-test] done\r\n\r\n");
        return;
    }

    for (UINT size = 64U; size <= 512U; size *= 2U)
    {
        for (UINT i = 0U; i < size; i++)
        {
            g_bufferWrite[i] = (uint8_t)(i ^ size);
            g_bufferRead[i] = 0U;
        }
        irq_before = g_usdhc3_irq_count;
        f_err = f_lseek(&file, 0U);
        if (!f_err)
        {
            f_err = f_write(&file, g_bufferWrite, size, &bytesWritten);
        }
        if (!f_err)
        {
            f_err = f_sync(&file);
        }
        if (!f_err)
        {
            f_err = f_lseek(&file, 0U);
        }
        if (!f_err)
        {
            f_err = f_read(&file, g_bufferRead, size, &bytesRead);
        }
        irq_after = g_usdhc3_irq_count;

        if (f_err || (bytesWritten != size) || (bytesRead != size) ||
            (memcmp(g_bufferWrite, g_bufferRead, size) != 0))
        {
            PRINTF("fatfs rw test: size=%u NG (err=%d wr=%u rd=%u)\r\n",
                   (unsigned)size, f_err, (unsigned)bytesWritten, (unsigned)bytesRead);
            break;
        }
        PRINTF("fatfs rw test: size=%u OK (irq +%u)\r\n",
               (unsigned)size, (unsigned)(irq_after - irq_before));
    }
    (void)f_close(&file);
    PRINTF("fatfs rw test: done\r\n");

    PRINTF("[Self-test] done\r\n\r\n");
}

static void mtx_test_task(VP_INT exinf)
{
    const uint32_t id = (uint32_t)exinf;
    FIL file;
    FRESULT f_err;
    UINT bytesWritten;
    char line[64];
    PRINTF("mtx_task%u: start\r\n", (unsigned)id);
    f_err = f_chdrive("0:");
    if (f_err != FR_OK)
    {
        PRINTF("mtx_task%u: f_chdrive NG err=%d\r\n", (unsigned)id, (int)f_err);
    }
    f_err = f_open(&file, g_mtx_test_path, FA_OPEN_APPEND | FA_WRITE);
    if (f_err != FR_OK)
    {
        PRINTF("mtx_task%u: f_open NG (%s) err=%d\r\n", (unsigned)id, g_mtx_test_path, (int)f_err);
    }
    if (f_err == FR_OK)
    {
        for (uint32_t i = 0U; i < 100U; i++)
        {
            (void)snprintf(line, sizeof(line), "T%u:%03u\r\n", (unsigned)id, (unsigned)i);
            f_err = f_write(&file, line, (UINT)strlen(line), &bytesWritten);
            if ((f_err != FR_OK) || (bytesWritten != (UINT)strlen(line)))
            {
                PRINTF("mtx_task%u: f_write NG err=%d wr=%u\r\n",
                       (unsigned)id, (int)f_err, (unsigned)bytesWritten);
                break;
            }
            /* Yield to encourage interleaving. */
            OSA_TimeDelay(1U);
        }
        (void)f_close(&file);
        PRINTF("mtx_task%u: done\r\n", (unsigned)id);
    }

    if (g_mtx_test_event_ready)
    {
        (void)OSA_EventSet(g_mtx_test_event, (id == 1U) ? 0x1U : 0x2U);
    }

}

static void mtx_test(void)
{
    osa_status_t st;
    osa_event_flags_t flags = 0U;
    ER_ID tskid1;
    ER_ID tskid2;
    const T_CTSK tsk1 = {
        TA_HLNG | TA_ACT | TA_FPU,
        (VP_INT)1,
        (FP)mtx_test_task,
        5,
        0x1000,
        0,
        "mtx_test_1"};
    const T_CTSK tsk2 = {
        TA_HLNG | TA_ACT | TA_FPU,
        (VP_INT)2,
        (FP)mtx_test_task,
        5,
        0x1000,
        0,
        "mtx_test_2"};

    PRINTF("\r\n[Mutex test] start\r\n");

    if (!g_mtx_test_event_ready)
    {
        st = OSA_EventCreate(g_mtx_test_event, 0U);
        if (st != KOSA_StatusSuccess)
        {
            PRINTF("mtx_test: event create NG (%u)\r\n", (unsigned)st);
            return;
        }
        g_mtx_test_event_ready = true;
    }
    (void)OSA_EventClear(g_mtx_test_event, 0x3U);
    (void)f_unlink(g_mtx_test_path);

    tskid1 = acre_tsk((T_CTSK *)&tsk1);
    tskid2 = acre_tsk((T_CTSK *)&tsk2);
    /* Yield to let tasks run. */
    OSA_TimeDelay(1U);

    st = OSA_EventWait(g_mtx_test_event, 0x3U, 1U, 10000U, &flags);
    if (st == KOSA_StatusSuccess)
    {
        PRINTF("[Mutex test] done (file=%s)\r\n\r\n", g_mtx_test_path);
    }
    else
    {
        PRINTF("[Mutex test] timeout/NG (%u, flags=0x%02x)\r\n\r\n",
               (unsigned)st, (unsigned)flags);
    }
}

int fatfs_task(VP_INT exinf)
{
    FRESULT error;
    DIR directory; /* Directory object */
    FILINFO fileInformation;
    UINT bytesWritten;
    UINT bytesRead;
    const TCHAR driverNumberBuffer[3U] = {MMCDISK + '0', ':', '/'};
    BYTE work[FF_MAX_SS];
    FRESULT result;

    //BOARD_InitHardware();
    BOARD_MMC_Config(&g_mmc, BOARD_SDMMC_MMC_HOST_IRQ_PRIORITY);

    __enable_irq();
    PRINTF("\r\nFATFS example to demonstrate how to use FATFS with MMC card.\r\n");

    /* Mount volume work area based on card. */
    result = f_mount(&g_fileSystem, driverNumberBuffer, 1U);

#if FF_USE_MKFS
    if (result == FR_NO_FILESYSTEM)
    {
        PRINTF("\r\nMake file system......The time may be long if the card capacity is big.\r\n");
        result = f_mkfs(driverNumberBuffer, 0, work, sizeof work);

        if (result)
        {
            PRINTF("Make file system failed.\r\n");
            return -1;
        }
    }
#endif /* FF_USE_MKFS */

    if (result != FR_OK)
    {
        PRINTF("Mount volume failed. result=%d\r\n", result);
        return -1;
    }
    else
    {
        PRINTF("Mount volume success.\r\n");
        mmc_print_bus_status();
    } 

#if (FF_FS_RPATH >= 2)
    error = f_chdrive((char const *)&driverNumberBuffer[0]);
    
    if (error)
    {
        PRINTF("Change drive failed.\r\n");
        return -1;
    }
#endif
    cli_prompt_loop();

    while (true)
    {
    }
}

void sample_fatfs_start(void)
{
    const T_CTSK ctsk_fatfs = {
        TA_HLNG | TA_ACT | TA_FPU,
        (VP_INT)0,
        (FP)fatfs_task,
        6,
        0x2000,
        0,
        "fatfs_task"};

    (void)acre_tsk((T_CTSK *)&ctsk_fatfs);
}
