/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Simple File System (Sample)
    Copyright (c) 2014, eForce Co., Ltd. All rights reserved.
    
    Version Information
      2014.03.18: Created
      2016.03.08: Add STS_EOF and STS_EOFI macros.
                  Add ffs_feof function.
      2016.04.06: Add ffs_rename function.
 ***************************************************************************/

#include "kernel.h"
#include "ffsys.h"
#include "net_hdr.h"
#include "net_strlib.h"

/* Conversion macros */
#define strlen    net_strlen
#define strcpy    net_strcpy
#define strcat    net_strcat
#define strcmp    net_strcmp
#define memcpy    net_memcpy

/* Time */
#define DIR_TIM_DEF_DATE     0x0221           /* Default date (1981-01-01) */
#define DIR_TIM_DEF_TIME     0x0000           /* Default time (00:00:00) */

/* Status */
#define STS_NONE     0                        /* No file */
#define STS_OPN      1                        /* Opened */
#define STS_CLS      2                        /* Closed */
#define STS_EOF      0x0080u                  /* End of file */
#define STS_EOFI     0x8000u                  /* End of file indicator */

/* Variables */
static FFS_FILE ffs_tbl[CFG_FFS_FILE_CNT];    /* File table */
static INT ffs_dir_cnt = 0;                   /* Number of open directories */
static struct ffs_dirent ffs_dir_ent;         /* Directory entry */
static INT ffs_dir_ent_id = 0;                /* Directory entry index */

ER ffs_ini(void)
{
    INT i;

    for (i = 0; i < CFG_FFS_FILE_CNT; i++) {
        ffs_tbl[i].sts = STS_NONE;
    }

    ffs_dir_cnt = 0;

    return E_OK;
}

FFS_FILE* ffs_fopen(const char* file_name, const char* mode)
{
    FFS_FILE* fs;
    INT i;
    FFS_FILE* fs_free;

    if (file_name == 0x00 || mode == 0x00) {
        return 0x00;
    }
    if (file_name[0] != CFG_FFS_DRV_NAME || file_name[1] != ':') {
        return 0x00;
    }
    if (strlen(file_name) < 4) {
        return 0x00;
    }

    fs = 0x00;
    if (mode[0] == 'r') {
        for (i = 0; i < CFG_FFS_FILE_CNT; i++) {
            if (ffs_tbl[i].sts != STS_NONE) {
                if (strcmp((const char*)ffs_tbl[i].name, file_name + 3) == 0) {
                    fs = &ffs_tbl[i];
                    break;
                }
            }
        }
    } else if (mode[0] == 'w') {
        fs_free = 0x00;
        for (i = 0; i < CFG_FFS_FILE_CNT; i++) {
            if (ffs_tbl[i].sts != STS_NONE) {
                if (strcmp((const char*)ffs_tbl[i].name, file_name + 3) == 0) {
                    fs = &ffs_tbl[i];
                    break;
                }
            } else {
                fs_free = &ffs_tbl[i];
            }
        }
        if (fs == 0x00 && fs_free != 0x00) {
            fs = fs_free;
            strcpy(fs->name, file_name + 3);
        }
        if (fs != 0x00) {
            fs->len = 0;
        }
    }

    if (fs != 0x00) {
        fs->sts = STS_OPN;
        fs->pos = 0;
    }

    return fs;
}

int ffs_fclose(FFS_FILE* fp)
{
    if (fp != 0x00) {
        fp->sts = STS_CLS;
    }

    return E_OK;
}

ffs_size_t ffs_fread(void* buf, ffs_size_t size, ffs_size_t n, FFS_FILE* fp)
{
    UW len;
    UW dat_len;

    if (fp == 0x00 || fp->sts != STS_OPN) {
        return 0;
    }
    if (buf == 0x00 || n == 0) {
        return 0;
    }

    len = fp->len - fp->pos;
    if (len == 0) {
        fp->sts |= STS_EOF;
        fp->sts |= STS_EOFI;
        return 0;
    }

    dat_len = size * n;
    if (len > dat_len) {
        len = dat_len;
    } else {
        n = len / size;
        len = size * n;
        fp->sts |= STS_EOF;
        fp->sts |= STS_EOFI;
    }
    memcpy(buf, (const void *)(fp->buf + fp->pos), len);
    fp->pos += len;

    return n;
}

ffs_size_t ffs_fwrite(const void* buf, ffs_size_t size, ffs_size_t n, FFS_FILE* fp)
{
    UW len;
    UW dat_len;

    if (fp == 0x00 || fp->sts != STS_OPN) {
        return 0;
    }
    if (buf == 0x00 || n == 0) {
        return 0;
    }

    len = (CFG_FFS_FILE_LEN) - fp->len;
    if (len == 0) {
        return 0;
    }

    dat_len = size * n;
    if (len > dat_len) {
        len = dat_len;
    } else {
        n = len / size;
        len = size * n;
    }
    memcpy(fp->buf + fp->pos, buf, len);
    fp->pos += len;
    fp->len += len;

    return n;
}

int ffs_fseek(FFS_FILE* fp, long offset, int from)
{
    if (fp == 0x00 || fp->sts != STS_OPN) {
        return -1;
    }
    if (from != FFS_SEEK_SET) {
        return -1;
    }

    fp->pos = offset;

    return 0;
}

int ffs_remove(const char* file_name)
{
    FFS_FILE* fp;

    if (file_name == 0x00) {
        return -1;
    }

    fp = ffs_fopen(file_name, "rb");
    if (fp == 0x00) {
        return -1;
    }
    fp->sts = STS_NONE;

    return 0;
}

int ffs_rename(const char* old_path, const char* new_path)
{
    FFS_FILE* fp;

    if ((!old_path) || (!new_path)) {
        return -1;
    }
    
    fp = ffs_fopen(new_path, "rb");
    if (fp != 0x00) {
        ffs_fclose(fp);
        return -1;
    }

    fp = ffs_fopen(old_path, "rb");
    if (fp == 0x00) {
        return -1;
    }
    ffs_fclose(fp);
    
    if (sizeof(fp->name) < strlen(new_path)) {
        return -1;
    }
    
    strcpy(fp->name, new_path + 3);
    return 0;
}


int ffs_mkdir(const char* dir_name, unsigned long st_mode)
{
    return -1;    /* No supported */
}

int ffs_rmdir(const char* dir_name)
{
    return -1;    /* No supported */
}

FFS_DIR* ffs_opendir(const char* dir_name)
{
    UW len;

    if (dir_name == 0x00) {
        return 0x00;
    }
    if (ffs_dir_cnt > 0) {
        return 0x00;
    }

    if (dir_name[0] == CFG_FFS_DRV_NAME) {
        len = strlen(dir_name);
        if (len > 3) {      /* Length of C:/ */
            return 0x00;    /* Sub directory is not supported */
        }
    }

    ffs_dir_cnt++;
    ffs_dir_ent_id = 0;

    return (FFS_DIR*)&ffs_dir_cnt;
}

struct ffs_dirent* ffs_readdir(FFS_DIR* dp)
{
    ER ercd;
    INT i;

    if (ffs_dir_cnt != 1) {
        return 0x00;
    }
    if (dp != (FFS_DIR*)&ffs_dir_cnt) {
        return 0x00;
    }

    ercd = E_OBJ;
    for (i = ffs_dir_ent_id; i < CFG_FFS_FILE_CNT; i++) {
        if (ffs_tbl[i].sts != STS_NONE) {
            ffs_dir_ent_id = i;
            ercd = E_OK;
            break;
        }
    }
    if (ercd != E_OK) {
        return 0x00;
    }

    ffs_dir_ent.d_reclen = sizeof(struct ffs_dirent);            /* Length of record */
    ffs_dir_ent.d_type = FFS_DT_ARCHIVE;                         /* Type */
    strcpy(ffs_dir_ent.d_name, (const char*)ffs_tbl[i].name);    /* Filename */

    ffs_dir_ent_id++;

    return &ffs_dir_ent;
}

int ffs_closedir(FFS_DIR* dp)
{
    ffs_dir_cnt = 0;

    return 0;
}

int ffs_stat(const char* file_name, struct ffs_stat* sts)
{
    FFS_FILE* fp;

    if (file_name == 0x00 || sts == 0x00) {
        return -1;
    }

    fp = ffs_fopen(file_name, "rb");
    if (fp == 0x00) {
        return -1;
    }

    /* Mode */
    sts->st_mode = FFS_S_IFREG | FFS_S_IRUSR | FFS_S_IWUSR;
    sts->st_size = fp->len;     /* Size (byte) */
    /* Last accessed time */
    sts->st_atime = DIR_TIM_DEF_DATE << 16;    
    /* Last modified time */
    sts->st_mtime = DIR_TIM_DEF_DATE << 16 | DIR_TIM_DEF_TIME;
    /* Creation time */
    sts->st_ctime = DIR_TIM_DEF_DATE << 16 | DIR_TIM_DEF_TIME;

    return 0;
}

int ffs_feof(FFS_FILE* fp)
{
    FFS_FILE* file;
    int res;

    /* End of file */

    if (fp == 0) {
        return 0;
    }

    file = (FFS_FILE*)fp;

    res = 0;
    if ((file->sts & STS_EOFI) != 0x00u) {
        res = FFS_EOF;
    }

    return res;
}
