/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Simple File System (Sample) Header
    Copyright (c) 2014, eForce Co., Ltd. All rights reserved.
    
    Version Information
      2014.03.18: Created
      2016.02.10: Add include files for warning avoidance
      2016.03.08: Add FFS_EOF and EOF and feof macros.
                  Add ffs_feof function.
      2016.04.06: Add ffs_rename function.
 ***************************************************************************/

#ifndef FFSYS_H
#define FFSYS_H
#ifdef __cplusplus
extern "C" {
#endif

#include "kernel.h"
#include "ffsys_cfg.h"

/* Macros */
#define FFS_EOF              -1                    /* End of file */
#define FFS_FILENAME_MAX     13                    /* Maximum length of short filename */
#define FFS_FOPEN_MAX        CFG_FFS_FILE_CNT      /* Number of open files at any time */
#define FFS_DIR_OPEN_MAX     1                     /* Number of open directory at any time */
#define FFS_PATH_MAX         260                   /* Maximum length of file path */

/* Type */
typedef unsigned int ffs_size_t;    /* Size */
typedef void FFS_DIR;               /* Directory handle */

/* File */
typedef struct t_ffs_file {
    UW len;                         /* Size */
    UW pos;                         /* Position */
    UW sts;                         /* Status */
    char name[FFS_PATH_MAX];        /* File name */
    char buf[CFG_FFS_FILE_LEN];     /* Data buffer */
} FFS_FILE;

/* Directory entry */
struct ffs_dirent {
    unsigned short d_reclen;      /* Length of this record */
    unsigned char d_type;         /* Type */
    char d_name[FFS_PATH_MAX];    /* Filename */
};

/* File status */
struct ffs_stat {
    unsigned long st_mode;     /* Mode */
    unsigned long st_size;     /* Size (byte) */
    unsigned long st_atime;    /* Last accessed time */
    unsigned long st_mtime;    /* Last modified time */
    unsigned long st_ctime;    /* Creation time */
};

/* Seek */
#define FFS_SEEK_SET    0    /* Begin of file */
#define FFS_SEEK_CUR    1    /* Current file position */
#define FFS_SEEK_END    2    /* End of file */

/* File type (d_type) */
#define FFS_DT_READ_ONLY    0x01    /* Read only */
#define FFS_DT_HIDDEN       0x02    /* Hidden */
#define FFS_DT_SYSTEM       0x04    /* System */
#define FFS_DT_VOL_ID       0x08    /* Volume ID */
#define FFS_DT_DIR          0x10    /* Directory */
#define FFS_DT_ARCHIVE      0x20    /* Archive */
#define FFS_DT_LONG_NAME    0x40    /* Long file name */
#define FFS_DT_REG          0x80    /* Regular file */

/* File mode (st_mode) */
#define FFS_S_IRUSR    0x00000400    /* Readable */
#define FFS_S_IWUSR    0x00000200    /* Writable */
#define FFS_S_IRO      0x00000001    /* Read only */
#define FFS_S_IHID     0x00000002    /* Hidden */
#define FFS_S_ISYS     0x00000004    /* System */
#define FFS_S_IFDIR    0x00000010    /* Directory */
#define FFS_S_IARC     0x00000020    /* Archive */
#define FFS_S_IFREG    0x00000080    /* Regular file */

/* Initialize function */
ER ffs_ini(void);

/* Standard functions */
FFS_FILE* ffs_fopen(const char*, const char*);
int ffs_fclose(FFS_FILE*);
ffs_size_t ffs_fread(void*, ffs_size_t, ffs_size_t, FFS_FILE*);
ffs_size_t ffs_fwrite(const void*, ffs_size_t, ffs_size_t, FFS_FILE*);
int ffs_fseek(FFS_FILE*, long, int);
int ffs_remove(const char*);
int ffs_rename(const char*, const char*);
int ffs_mkdir(const char*, unsigned long);
int ffs_rmdir(const char*);
FFS_DIR* ffs_opendir(const char*);
struct ffs_dirent* ffs_readdir(FFS_DIR*);
int ffs_closedir(FFS_DIR*);
int ffs_stat(const char*, struct ffs_stat*);
int ffs_feof(FFS_FILE*);

/* Conversion macros */
#if !defined(CFG_FFS_CNV_DIS)

#ifndef CFG_FFS_EOF_DIS
#define EOF    FFS_EOF
#endif
#if !defined(CFG_FFS_FILENAME_MAX_DIS)
#define FILENAME_MAX    FFS_FILENAME_MAX
#endif
#if !defined(CFG_FFS_FOPEN_MAX_DIS)
#define FOPEN_MAX    FFS_FOPEN_MAX
#endif
#if !defined(CFG_FFS_DIR_OPEN_MAX_DIS)
#define DIR_OPEN_MAX    FFS_DIR_OPEN_MAX
#endif
#if !defined(CFG_FFS_PATH_MAX_DIS)
#define PATH_MAX    FFS_PATH_MAX
#endif
#if !defined(CFG_FFS_SEEK_DIS)
#define SEEK_CUR    FFS_SEEK_CUR
#define SEEK_SET    FFS_SEEK_SET
#define SEEK_END    FFS_SEEK_END
#endif
#if !defined(CFG_FFS_SIZE_T_DIS)
#define size_t    ffs_size_t
#endif
#if !defined(CFG_FFS_FILE_DIS)
#define FILE    FFS_FILE
#endif
#if !defined(CFG_FFS_DIR_DIS)
#define DIR    FFS_DIR
#endif
#if !defined(CFG_FFS_DIRENT_DIS)
#define DT_READ_ONLY    FFS_DT_READ_ONLY
#define DT_HIDDEN       FFS_DT_HIDDEN
#define DT_SYSTEM       FFS_DT_SYSTEM
#define DT_VOL_ID       FFS_DT_VOL_ID
#define DT_DIR          FFS_DT_DIR
#define DT_ARCHIVE      FFS_DT_ARCHIVE
#define DT_LONG_NAME    FFS_DT_LONG_NAME
#define DT_REG          FFS_DT_REG
#define dirent          ffs_dirent
#endif
#if !defined(CFG_FFS_STAT_DIS)
#define S_IRUSR    FFS_S_IRUSR
#define S_IWUSR    FFS_S_IWUSR
#define S_IRO      FFS_S_IRO
#define S_IHID     FFS_S_IHID
#define S_ISYS     FFS_S_ISYS
#define S_IFDIR    FFS_S_IFDIR
#define S_IARC     FFS_S_IARC
#define S_IFREG    FFS_S_IFREG
#endif
#if !defined(CFG_FFS_C_FUNC_DIS)
#define fopen     ffs_fopen
#define fclose    ffs_fclose
#define fread     ffs_fread
#define fwrite    ffs_fwrite
#define fseek     ffs_fseek
#define remove    ffs_remove
#define rename    ffs_rename
#define feof      ffs_feof
#endif
#if !defined(CFG_FFS_POS_FUNC_DIS)
#define mkdir       ffs_mkdir
#define rmdir       ffs_rmdir
#define opendir     ffs_opendir
#define readdir     ffs_readdir
#define closedir    ffs_closedir
#define stat        ffs_stat
#endif

#endif

#ifdef __cplusplus
}
#endif
#endif

