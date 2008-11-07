#ifndef CONFIG_COMPAT_COMPAT_H
#define CONFIG_COMPAT_COMPAT_H

#define gettext
#define _gettext
#define _dgettext

#include <sys/types.h>
#include <sys/endian.h>

typedef off_t  __off64_t;

#define __LITTLE_ENDIAN (1234)
#define __BIG_ENDIAN    (4321)
#define __BYTE_ORDER    __LITTLE_ENDIAN

#include <stddef.h>
#include <locale.h> //LC_MESSAGES
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

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

static inline size_t strnlen (const char *__string, size_t __maxlen)
{
	int len = 0;
	while (__maxlen-- && *__string++)
		len++;
	return len;
}

static inline void *mempcpy (void * __dest, const void * __src, size_t __n)
{
	memcpy(__dest, __src, __n);
	return ((char *)__dest) + __n;
}

#define __mempcpy mempcpy

static inline wchar_t *wmempcpy (wchar_t *__restrict __s1, __const wchar_t *__restrict __s2, size_t __n)
{
	error(EXIT_FAILURE, 0, "%s not implemented!", __FUNCTION__);
	return NULL;
}


#define bswap_16 bswap16
#define bswap_32 bswap32
#define bswap_64 bswap64

extern int ___libelf_fill_byte;

#endif /*CONFIG_COMPAT_DARWIN_H*/
