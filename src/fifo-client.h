#ifndef FIFO_CLIENT_H
#define FIFO_CLIENT_H

#include <errno.h>

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
  (__extension__ \
    ({ long int __result; \
       do __result = (long int) (expression); \
       while (__result == -1L && (errno == EINTR || errno == EAGAIN)); \
       __result; }))
#endif

#endif
