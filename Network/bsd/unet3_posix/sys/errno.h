/* dummy */

#if defined(__RENESAS__)  /* CubeSuite+ */
  #define NOUSE_ERRNO_DEFINED
#else
  /* Other compiler */
#endif

#if !defined(NOUSE_ERRNO_DEFINED)
  #define errno  unet_errno
#endif

extern int unet_errno;
int get_errno(void);

#include "unet3_sys.h"

