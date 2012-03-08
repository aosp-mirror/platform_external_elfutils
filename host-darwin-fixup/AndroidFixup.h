#ifndef ANDROID_FIXUP_H
#define ANDROID_FIXUP_H

#define off_t loff_t
#define off64_t loff_t

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <locale.h> //LC_MESSAGES

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp) ({         \
    typeof (exp) _rc;                      \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1070
static inline size_t strnlen (const char *__string, size_t __maxlen)
{
        int len = 0;
        while (__maxlen-- && *__string++)
                len++;
        return len;
}
#endif

static inline void *mempcpy (void * __dest, const void * __src, size_t __n)
{
        memcpy(__dest, __src, __n);
        return ((char *)__dest) + __n;
}

#define __mempcpy mempcpy

#define dgettext(domainname, msgid)  dcgettext (domainname, msgid, LC_MESSAGES)

static inline void __attribute__((noreturn)) error(int status, int errnum, const char *fmt, ...)
{
        va_list lst;
        va_start(lst, fmt);
        vfprintf(stderr, fmt, lst);
        fprintf(stderr, "error %d: %s\n", errnum, strerror(errno));
        va_end(lst);
        exit(status);
}

static inline char *dcgettext (char *__domainname, char *__msgid, int __category)
{
        error(EXIT_FAILURE, 0, "%s not implemented!", __FUNCTION__);
        return NULL;
}

#endif /* ANDROID_FIXUP_H */
