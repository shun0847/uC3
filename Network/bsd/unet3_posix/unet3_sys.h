/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK
    BSD socket wrapper/System definitions
    Copyright (c) 2014-2015, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.04.28: Created
      2015.03.20: Modified to support uNet3/Compact.
      2016.06.08: Add FIONREAD option for ioctl.
 ***************************************************************************/

#ifndef     _UNET_SYS_H
#define     _UNET_SYS_H

#include "unet3_cfg.h"

/* for basic typedef */

/* sys/times.h  */

struct unet3_tms {
    unsigned int tms_utime;      /* user time */
    unsigned int tms_stime;      /* system time */
    unsigned int tms_cutime;     /* user time, children */
    unsigned int tms_cstime;     /* system time, children */
};


/* sys/time.h  */

typedef struct unet3_timeval {
  long    tv_sec;
  long    tv_usec;
}unet3_timeval;


/* sys/select.h */
#define   FD_SET_MAX          (BSD_SOCKET_MAX+31)/32

typedef struct unet3_fd_set {
    unsigned int  fds[FD_SET_MAX];
}unet3_fd_set;

#if !(SYS_PLATFORM)
#define tms       unet3_tms
#define timeval   unet3_timeval
#define fd_set    unet3_fd_set
#endif

int unet3_select(int nfds, unet3_fd_set *readfds, unet3_fd_set *writefds, unet3_fd_set *exceptfds, struct unet3_timeval *timeout);

/* macro */
#define     unet3_FD_SET(fd, fdset)     ((fdset)->fds[(fd-1)/32] |=  (1 << ((fd-1)%32)))
#define     unet3_FD_CLR(fd, fdset)     ((fdset)->fds[(fd-1)/32] &= ~(1 << ((fd-1)%32)))
#define     unet3_FD_ISSET(fd, fdset)   ((fdset)->fds[(fd-1)/32] &   (1 << ((fd-1)%32)))
#define     unet3_FD_ZERO(set)          net_memset((set)->fds, 0, sizeof(unet3_fd_set))


#if !(SYS_PLATFORM)
#define select        unet3_select
#define ioctl         unet3_ioctl
#define fd_set        unet3_fd_set
#define FD_CLR        unet3_FD_CLR
#define FD_ISSET      unet3_FD_ISSET
#define FD_SET        unet3_FD_SET
#define FD_ZERO       unet3_FD_ZERO
#endif


/* sys/ioctl.h */
int unet3_ioctl(int d, int request, ...);

#define UNET3_FIONBIO       0
#define UNET3_FIONREAD      1

#if !(SYS_PLATFORM)
#define ioctl         unet3_ioctl
#define FIONBIO       UNET3_FIONBIO
#define FIONREAD      UNET3_FIONREAD
#endif


#if defined(__RENESAS__)  /* CubeSuite+ */
#include <string.h>  /* use default errno defined */

/* non default errno settings */
#define ENOFFSET                    0x1000
#define ENOTBLK                     (ENOFFSET + 15)
#define ETXTBSY                     (ENOFFSET + 26)
#define EWOULDBLOCK                 (ENOFFSET + 35)
#define EALREADY                    (ENOFFSET + 37)
#define ENOTSOCK                    (ENOFFSET + 38)
#define EDESTADDRREQ                (ENOFFSET + 39)
#define EPROTOTYPE                  (ENOFFSET + 41)
#define ENOPROTOOPT                 (ENOFFSET + 42)
#define EPROTONOSUPPORT             (ENOFFSET + 43)
#define ESOCKTNOSUPPORT             (ENOFFSET + 44)
#define EOPNOTSUPP                  (ENOFFSET + 45)
#define EPFNOSUPPORT                (ENOFFSET + 46)
#define EAFNOSUPPORT                (ENOFFSET + 47)
#define EADDRINUSE                  (ENOFFSET + 48)
#define EADDRNOTAVAIL               (ENOFFSET + 49)
#define ENETDOWN                    (ENOFFSET + 50)
#define ENETUNREACH                 (ENOFFSET + 51)
#define ENETRESET                   (ENOFFSET + 52)
#define ECONNABORTED                (ENOFFSET + 53)
#define ECONNRESET                  (ENOFFSET + 54)
#define ENOBUFS                     (ENOFFSET + 55)
#define EISCONN                     (ENOFFSET + 56)
#define ENOTCONN                    (ENOFFSET + 57)
#define ESHUTDOWN                   (ENOFFSET + 58)
#define ETOOMANYREFS                (ENOFFSET + 59)
//#define ETIMEDOUT                 
#define ECONNREFUSED                (ENOFFSET + 61)
//#define ENAMETOOLONG              
#define EHOSTDOWN                   (ENOFFSET + 64)
#define EHOSTUNREACH                (ENOFFSET + 65)
//#define ENOTEMPTY                 
#define EPROCLIM                    (ENOFFSET + 67)
#define EUSERS                      (ENOFFSET + 68)
#define EDQUOT                      (ENOFFSET + 69)
#define ESTALE                      (ENOFFSET + 70)
#define EREMOTE                     (ENOFFSET + 71)
#define EBADRPC                     (ENOFFSET + 72)
#define ERPCMISMATCH                (ENOFFSET + 73)
#define EPROGUNAVAIL                (ENOFFSET + 74)
#define EPROGMISMATCH               (ENOFFSET + 75)
#define EPROCUNAVAIL                (ENOFFSET + 76)
//#define ENOLCK                    
//#define ENOSYS                    
#define EFTYPE                      (ENOFFSET + 79)
#define EAUTH                       (ENOFFSET + 80)
#define ENEEDAUTH                   (ENOFFSET + 81)
#define ELAST                       (ENOFFSET + 81)
#define ERESTARTa                   -1
#define EJUSTRETURN                 -2

/* sys/errno.h */
#define UNET3_ER_EPERM              EPERM           /* Operation not permitted */
#define UNET3_ER_ENOENT             ENOENT          /* No such file or directory */
#define UNET3_ER_ESRCH              ESRCH           /* No such process */
#define UNET3_ER_EINTR              EINTR           /* Interrupted system call */
#define UNET3_ER_EIO                EIO             /* Input/output error */
#define UNET3_ER_ENXIO              ENXIO           /* Device not configured */
#define UNET3_ER_E2BIG              E2BIG           /* Argument list too long */
#define UNET3_ER_ENOEXEC            ENOEXEC         /* Exec format error */
#define UNET3_ER_EBADF              EBADF           /* Bad file descriptor */
#define UNET3_ER_ECHILD             ECHILD          /* No child processes */
#define UNET3_ER_EDEADLK            EDEADLK         /* Resource deadlock avoided */
#define UNET3_ER_ENOMEM             ENOMEM          /* Cannot allocate memory */
#define UNET3_ER_EACCES             EACCES          /* Permission denied */
#define UNET3_ER_EFAULT             EFAULT          /* Bad address */
#define UNET3_ER_ENOTBLK            ENOTBLK         /* Block device required */
#define UNET3_ER_EBUSY              EBUSY           /* Device busy */
#define UNET3_ER_EEXIST             EEXIST          /* File exists */
#define UNET3_ER_EXDEV              EXDEV           /* Cross-device link */
#define UNET3_ER_ENODEV             ENODEV          /* Operation not supported by device */
#define UNET3_ER_ENOTDIR            ENOTDIR         /* Not a directory */
#define UNET3_ER_EISDIR             EISDIR          /* Is a directory */
#define UNET3_ER_EINVAL             EINVAL          /* Invalid argument */
#define UNET3_ER_ENFILE             ENFILE          /* Too many open files in system */
#define UNET3_ER_EMFILE             EMFILE          /* Too many open files */
#define UNET3_ER_ENOTTY             ENOTTY          /* Inappropriate ioctl for device */
#define UNET3_ER_ETXTBSY            ETXTBSY         /* Text file busy */
#define UNET3_ER_EFBIG              EFBIG           /* File too large */
#define UNET3_ER_ENOSPC             ENOSPC          /* No space left on device */
#define UNET3_ER_ESPIPE             ESPIPE          /* Illegal seek */
#define UNET3_ER_EROFS              EROFS           /* Read-only file system */
#define UNET3_ER_EMLINK             EMLINK          /* Too many links */
#define UNET3_ER_EPIPE              EPIPE           /* Broken pipe */
#define UNET3_ER_EDOM               EDOM            /* Numerical argument out of domain */
#define UNET3_ER_ERANGE             ERANGE          /* Result too large */
#define UNET3_ER_EAGAIN             EAGAIN          /* Resource temporarily unavailable */
#define UNET3_ER_EWOULDBLOCK UNET3_ER_EAGAIN        /* Operation would block */
#define UNET3_ER_EINPROGRESS        EINPROGRESS     /* Operation now in progress */
#define UNET3_ER_EALREADY           EALREADY        /* Operation already in progress */
#define UNET3_ER_ENOTSOCK           ENOTSOCK        /* Socket operation on non-socket */
#define UNET3_ER_EDESTADDRREQ       EDESTADDRREQ    /* Destination address required */
#define UNET3_ER_EMSGSIZE           EMSGSIZE        /* Message too long */
#define UNET3_ER_EPROTOTYPE         EPROTOTYPE      /* Protocol wrong type for socket */
#define UNET3_ER_ENOPROTOOPT        ENOPROTOOPT     /* Protocol not available */
#define UNET3_ER_EPROTONOSUPPORT    EPROTONOSUPPORT /* Protocol not supported */
#define UNET3_ER_ESOCKTNOSUPPORT    ESOCKTNOSUPPORT /* Socket type not supported */
#define UNET3_ER_EOPNOTSUPP         EOPNOTSUPP      /* Operation not supported */
#define UNET3_ER_EPFNOSUPPORT       EPFNOSUPPORT    /* Protocol family not supported */
#define UNET3_ER_EAFNOSUPPORT       EAFNOSUPPORT    /* Address family not supported by protocol family */
#define UNET3_ER_EADDRINUSE         EADDRINUSE      /* Address already in use */
#define UNET3_ER_EADDRNOTAVAIL      EADDRNOTAVAIL   /* Can't assign requested address */
#define UNET3_ER_ENETDOWN           ENETDOWN        /* Network is down */
#define UNET3_ER_ENETUNREACH        ENETUNREACH     /* Network is unreachable */
#define UNET3_ER_ENETRESET          ENETRESET       /* Network dropped connection on reset */
#define UNET3_ER_ECONNABORTED       ECONNABORTED    /* Software caused connection abort */
#define UNET3_ER_ECONNRESET         ECONNRESET      /* Connection reset by peer */
#define UNET3_ER_ENOBUFS            ENOBUFS         /* No buffer space available */
#define UNET3_ER_EISCONN            EISCONN         /* Socket is already connected */
#define UNET3_ER_ENOTCONN           ENOTCONN        /* Socket is not connected */
#define UNET3_ER_ESHUTDOWN          ESHUTDOWN       /* Can't send after socket shutdown */
#define UNET3_ER_ETOOMANYREFS       ETOOMANYREFS    /* Too many references: can't splice */
#define UNET3_ER_ETIMEDOUT          ETIMEDOUT       /* Operation timed out */
#define UNET3_ER_ECONNREFUSED       ECONNREFUSED    /* Connection refused */
#define UNET3_ER_ENAMETOOLONG       ENAMETOOLONG    /* File name too long */
#define UNET3_ER_EHOSTDOWN          EHOSTDOWN       /* Host is down */
#define UNET3_ER_EHOSTUNREACH       EHOSTUNREACH    /* No route to host */
#define UNET3_ER_ENOTEMPTY          ENOTEMPTY       /* Directory not empty */
#define UNET3_ER_EPROCLIM           EPROCLIM        /* Too many processes */
#define UNET3_ER_EUSERS             EUSERS          /* Too many users */
#define UNET3_ER_EDQUOT             EDQUOT          /* Disc quota exceeded */
#define UNET3_ER_ESTALE             ESTALE          /* Stale NFS file handle */
#define UNET3_ER_EREMOTE            EREMOTE         /* Too many levels of remote in path */
#define UNET3_ER_EBADRPC            EBADRPC         /* RPC struct is bad */
#define UNET3_ER_ERPCMISMATCH       ERPCMISMATCH    /* RPC version wrong */
#define UNET3_ER_EPROGUNAVAIL       EPROGUNAVAIL    /* RPC prog. not avail */
#define UNET3_ER_EPROGMISMATCH      EPROGMISMATCH   /* Program version wrong */
#define UNET3_ER_EPROCUNAVAIL       EPROCUNAVAIL    /* Bad procedure for program */
#define UNET3_ER_ENOLCK             ENOLCK          /* No locks available */
#define UNET3_ER_ENOSYS             ENOSYS          /* Function not implemented */
#define UNET3_ER_EFTYPE             EFTYPE          /* Inappropriate file type or format */
#define UNET3_ER_EAUTH              EAUTH           /* Authentication error */
#define UNET3_ER_ENEEDAUTH          ENEEDAUTH       /* Need authenticator */
#define UNET3_ER_ELAST              ELAST           /* Must be equal largest errno */
#define UNET3_ER_ERESTARTa          -1   /* restart syscall */
#define UNET3_ER_EJUSTRETURN        -2   /* don't modify regs, just return */


#else   /* defined(__RENESAS__) */

/* sys/errno.h */
#define UNET3_ER_EPERM              1    /* Operation not permitted */
#define UNET3_ER_ENOENT             2    /* No such file or directory */
#define UNET3_ER_ESRCH              3    /* No such process */
#define UNET3_ER_EINTR              4    /* Interrupted system call */
#define UNET3_ER_EIO                5    /* Input/output error */
#define UNET3_ER_ENXIO              6    /* Device not configured */
#define UNET3_ER_E2BIG              7    /* Argument list too long */
#define UNET3_ER_ENOEXEC            8    /* Exec format error */
#define UNET3_ER_EBADF              9    /* Bad file descriptor */
#define UNET3_ER_ECHILD             10   /* No child processes */
#define UNET3_ER_EDEADLK            11   /* Resource deadlock avoided */
#define UNET3_ER_ENOMEM             12   /* Cannot allocate memory */
#define UNET3_ER_EACCES             13   /* Permission denied */
#define UNET3_ER_EFAULT             14   /* Bad address */
#define UNET3_ER_ENOTBLK            15   /* Block device required */
#define UNET3_ER_EBUSY              16   /* Device busy */
#define UNET3_ER_EEXIST             17   /* File exists */
#define UNET3_ER_EXDEV              18   /* Cross-device link */
#define UNET3_ER_ENODEV             19   /* Operation not supported by device */
#define UNET3_ER_ENOTDIR            20   /* Not a directory */
#define UNET3_ER_EISDIR             21   /* Is a directory */
#define UNET3_ER_EINVAL             22   /* Invalid argument */
#define UNET3_ER_ENFILE             23   /* Too many open files in system */
#define UNET3_ER_EMFILE             24   /* Too many open files */
#define UNET3_ER_ENOTTY             25   /* Inappropriate ioctl for device */
#define UNET3_ER_ETXTBSY            26   /* Text file busy */
#define UNET3_ER_EFBIG              27   /* File too large */
#define UNET3_ER_ENOSPC             28   /* No space left on device */
#define UNET3_ER_ESPIPE             29   /* Illegal seek */
#define UNET3_ER_EROFS              30   /* Read-only file system */
#define UNET3_ER_EMLINK             31   /* Too many links */
#define UNET3_ER_EPIPE              32   /* Broken pipe */
#define UNET3_ER_EDOM               33   /* Numerical argument out of domain */
#define UNET3_ER_ERANGE             34   /* Result too large */
#define UNET3_ER_EAGAIN             35   /* Resource temporarily unavailable */
#define UNET3_ER_EWOULDBLOCK UNET3_ER_EAGAIN /* Operation would block */
#define UNET3_ER_EINPROGRESS        36   /* Operation now in progress */
#define UNET3_ER_EALREADY           37   /* Operation already in progress */
#define UNET3_ER_ENOTSOCK           38   /* Socket operation on non-socket */
#define UNET3_ER_EDESTADDRREQ       39   /* Destination address required */
#define UNET3_ER_EMSGSIZE           40   /* Message too long */
#define UNET3_ER_EPROTOTYPE         41   /* Protocol wrong type for socket */
#define UNET3_ER_ENOPROTOOPT        42   /* Protocol not available */
#define UNET3_ER_EPROTONOSUPPORT    43   /* Protocol not supported */
#define UNET3_ER_ESOCKTNOSUPPORT    44   /* Socket type not supported */
#define UNET3_ER_EOPNOTSUPP         45   /* Operation not supported */
#define UNET3_ER_EPFNOSUPPORT       46   /* Protocol family not supported */
#define UNET3_ER_EAFNOSUPPORT       47   /* Address family not supported by protocol family */
#define UNET3_ER_EADDRINUSE         48   /* Address already in use */
#define UNET3_ER_EADDRNOTAVAIL      49   /* Can't assign requested address */
#define UNET3_ER_ENETDOWN           50   /* Network is down */
#define UNET3_ER_ENETUNREACH        51   /* Network is unreachable */
#define UNET3_ER_ENETRESET          52   /* Network dropped connection on reset */
#define UNET3_ER_ECONNABORTED       53   /* Software caused connection abort */
#define UNET3_ER_ECONNRESET         54   /* Connection reset by peer */
#define UNET3_ER_ENOBUFS            55   /* No buffer space available */
#define UNET3_ER_EISCONN            56   /* Socket is already connected */
#define UNET3_ER_ENOTCONN           57   /* Socket is not connected */
#define UNET3_ER_ESHUTDOWN          58   /* Can't send after socket shutdown */
#define UNET3_ER_ETOOMANYREFS       59   /* Too many references: can't splice */
#define UNET3_ER_ETIMEDOUT          60   /* Operation timed out */
#define UNET3_ER_ECONNREFUSED       61   /* Connection refused */
#define UNET3_ER_ENAMETOOLONG       63   /* File name too long */
#define UNET3_ER_EHOSTDOWN          64   /* Host is down */
#define UNET3_ER_EHOSTUNREACH       65   /* No route to host */
#define UNET3_ER_ENOTEMPTY          66   /* Directory not empty */
#define UNET3_ER_EPROCLIM           67   /* Too many processes */
#define UNET3_ER_EUSERS             68   /* Too many users */
#define UNET3_ER_EDQUOT             69   /* Disc quota exceeded */
#define UNET3_ER_ESTALE             70   /* Stale NFS file handle */
#define UNET3_ER_EREMOTE            71   /* Too many levels of remote in path */
#define UNET3_ER_EBADRPC            72   /* RPC struct is bad */
#define UNET3_ER_ERPCMISMATCH       73   /* RPC version wrong */
#define UNET3_ER_EPROGUNAVAIL       74   /* RPC prog. not avail */
#define UNET3_ER_EPROGMISMATCH      75   /* Program version wrong */
#define UNET3_ER_EPROCUNAVAIL       76   /* Bad procedure for program */
#define UNET3_ER_ENOLCK             77   /* No locks available */
#define UNET3_ER_ENOSYS             78   /* Function not implemented */
#define UNET3_ER_EFTYPE             79   /* Inappropriate file type or format */
#define UNET3_ER_EAUTH              80   /* Authentication error */
#define UNET3_ER_ENEEDAUTH          81   /* Need authenticator */
#define UNET3_ER_ELAST              81   /* Must be equal largest errno */
#define UNET3_ER_ERESTARTa          -1   /* restart syscall */
#define UNET3_ER_EJUSTRETURN        -2   /* don't modify regs, just return */

#if !(SYS_PLATFORM)
#define EPERM                       UNET3_ER_EPERM
#define ENOENT                      UNET3_ER_ENOENT
#define ESRCH                       UNET3_ER_ESRCH
#define EINTR                       UNET3_ER_EINTR
#define EIO                         UNET3_ER_EIO
#define ENXIO                       UNET3_ER_ENXIO
#define E2BIG                       UNET3_ER_E2BIG
#define ENOEXEC                     UNET3_ER_ENOEXEC
#define EBADF                       UNET3_ER_EBADF
#define ECHILD                      UNET3_ER_ECHILD
#define EDEADLK                     UNET3_ER_EDEADLK
#define ENOMEM                      UNET3_ER_ENOMEM
#define EACCES                      UNET3_ER_EACCES
#define EFAULT                      UNET3_ER_EFAULT
#define ENOTBLK                     UNET3_ER_ENOTBLK
#define EBUSY                       UNET3_ER_EBUSY
#define EEXIST                      UNET3_ER_EEXIST
#define EXDEV                       UNET3_ER_EXDEV
#define ENODEV                      UNET3_ER_ENODEV
#define ENOTDIR                     UNET3_ER_ENOTDIR
#define EISDIR                      UNET3_ER_EISDIR
#define EINVAL                      UNET3_ER_EINVAL
#define ENFILE                      UNET3_ER_ENFILE
#define EMFILE                      UNET3_ER_EMFILE
#define ENOTTY                      UNET3_ER_ENOTTY
#define ETXTBSY                     UNET3_ER_ETXTBSY
#define EFBIG                       UNET3_ER_EFBIG
#define ENOSPC                      UNET3_ER_ENOSPC
#define ESPIPE                      UNET3_ER_ESPIPE
#define EROFS                       UNET3_ER_EROFS
#define EMLINK                      UNET3_ER_EMLINK
#define EPIPE                       UNET3_ER_EPIPE
#define EDOM                        UNET3_ER_EDOM
#define ERANGE                      UNET3_ER_ERANGE
#define EAGAIN                      UNET3_ER_EAGAIN
#define EWOULDBLOCK                 UNET3_ER_EWOULDBLOCK
#define EINPROGRESS                 UNET3_ER_EINPROGRESS
#define EALREADY                    UNET3_ER_EALREADY
#define ENOTSOCK                    UNET3_ER_ENOTSOCK
#define EDESTADDRREQ                UNET3_ER_EDESTADDRREQ
#define EMSGSIZE                    UNET3_ER_EMSGSIZE
#define EPROTOTYPE                  UNET3_ER_EPROTOTYPE
#define ENOPROTOOPT                 UNET3_ER_ENOPROTOOPT
#define EPROTONOSUPPORT             UNET3_ER_EPROTONOSUPPORT
#define ESOCKTNOSUPPORT             UNET3_ER_ESOCKTNOSUPPORT
#define EOPNOTSUPP                  UNET3_ER_EOPNOTSUPP
#define EPFNOSUPPORT                UNET3_ER_EPFNOSUPPORT
#define EAFNOSUPPORT                UNET3_ER_EAFNOSUPPORT
#define EADDRINUSE                  UNET3_ER_EADDRINUSE
#define EADDRNOTAVAIL               UNET3_ER_EADDRNOTAVAIL
#define ENETDOWN                    UNET3_ER_ENETDOWN
#define ENETUNREACH                 UNET3_ER_ENETUNREACH
#define ENETRESET                   UNET3_ER_ENETRESET
#define ECONNABORTED                UNET3_ER_ECONNABORTED
#define ECONNRESET                  UNET3_ER_ECONNRESET
#define ENOBUFS                     UNET3_ER_ENOBUFS
#define EISCONN                     UNET3_ER_EISCONN
#define ENOTCONN                    UNET3_ER_ENOTCONN
#define ESHUTDOWN                   UNET3_ER_ESHUTDOWN
#define ETOOMANYREFS                UNET3_ER_ETOOMANYREFS
#define ETIMEDOUT                   UNET3_ER_ETIMEDOUT
#define ECONNREFUSED                UNET3_ER_ECONNREFUSED
#define ENAMETOOLONG                UNET3_ER_ENAMETOOLONG
#define EHOSTDOWN                   UNET3_ER_EHOSTDOWN
#define EHOSTUNREACH                UNET3_ER_EHOSTUNREACH
#define ENOTEMPTY                   UNET3_ER_ENOTEMPTY
#define EPROCLIM                    UNET3_ER_EPROCLIM
#define EUSERS                      UNET3_ER_EUSERS
#define EDQUOT                      UNET3_ER_EDQUOT
#define ESTALE                      UNET3_ER_ESTALE
#define EREMOTE                     UNET3_ER_EREMOTE
#define EBADRPC                     UNET3_ER_EBADRPC
#define ERPCMISMATCH                UNET3_ER_ERPCMISMATCH
#define EPROGUNAVAIL                UNET3_ER_EPROGUNAVAIL
#define EPROGMISMATCH               UNET3_ER_EPROGMISMATCH
#define EPROCUNAVAIL                UNET3_ER_EPROCUNAVAIL
#define ENOLCK                      UNET3_ER_ENOLCK
#define ENOSYS                      UNET3_ER_ENOSYS
#define EFTYPE                      UNET3_ER_EFTYPE
#define EAUTH                       UNET3_ER_EAUTH
#define ENEEDAUTH                   UNET3_ER_ENEEDAUTH
#define ELAST                       UNET3_ER_ELAST
#define ERESTARTa                   UNET3_ER_ERESTARTa
#define EJUSTRETURN                 UNET3_ER_EJUSTRETURN
#endif

#endif  /* defined(__RENESAS__) */

#endif
