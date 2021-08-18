/* Retrieve ELF / DWARF / source files from the debuginfod.
   Copyright (C) 2019-2021 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */


/* cargo-cult from libdwfl linux-kernel-modules.c */
/* In case we have a bad fts we include this before config.h because it
   can't handle _FILE_OFFSET_BITS.
   Everything we need here is fine if its declarations just come first.
   Also, include sys/types.h before fts. On some systems fts.h is not self
   contained. */
#ifdef BAD_FTS
  #include <sys/types.h>
  #include <fts.h>
#endif

#include "config.h"
#include "debuginfod.h"
#include "system.h"
#include <errno.h>
#include <stdlib.h>

/* We might be building a bootstrap dummy library, which is really simple. */
#ifdef DUMMY_LIBDEBUGINFOD

debuginfod_client *debuginfod_begin (void) { errno = ENOSYS; return NULL; }
int debuginfod_find_debuginfo (debuginfod_client *c, const unsigned char *b,
                               int s, char **p) { return -ENOSYS; }
int debuginfod_find_executable (debuginfod_client *c, const unsigned char *b,
                                int s, char **p) { return -ENOSYS; }
int debuginfod_find_source (debuginfod_client *c, const unsigned char *b,
                            int s, const char *f, char **p)  { return -ENOSYS; }
void debuginfod_set_progressfn(debuginfod_client *c,
			       debuginfod_progressfn_t fn) { }
void debuginfod_set_verbose_fd(debuginfod_client *c, int fd) { }
void debuginfod_set_user_data (debuginfod_client *c, void *d) { }
void* debuginfod_get_user_data (debuginfod_client *c) { return NULL; }
const char* debuginfod_get_url (debuginfod_client *c) { return NULL; }
int debuginfod_add_http_header (debuginfod_client *c,
				const char *h) { return -ENOSYS; }
void debuginfod_end (debuginfod_client *c) { }

#else /* DUMMY_LIBDEBUGINFOD */

#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <fts.h>
#include <regex.h>
#include <string.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <time.h>
#include <utime.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <curl/curl.h>

/* If fts.h is included before config.h, its indirect inclusions may not
   give us the right LFS aliases of these functions, so map them manually.  */
#ifdef BAD_FTS
  #ifdef _FILE_OFFSET_BITS
    #define open open64
    #define fopen fopen64
  #endif
#else
  #include <sys/types.h>
  #include <fts.h>
#endif

struct debuginfod_client
{
  /* Progress/interrupt callback function. */
  debuginfod_progressfn_t progressfn;

  /* Stores user data. */
  void* user_data;

  /* Stores current/last url, if any. */
  char* url;

  /* Accumulates outgoing http header names/values. */
  int user_agent_set_p; /* affects add_default_headers */
  struct curl_slist *headers;

  /* Flags the default_progressfn having printed something that
     debuginfod_end needs to terminate. */
  int default_progressfn_printed_p;

  /* File descriptor to output any verbose messages if > 0.  */
  int verbose_fd;

  /* Maintain a long-lived curl multi-handle, which keeps a
     connection/tls/dns cache to recently seen servers. */
  CURLM *server_mhandle;
  
  /* Can contain all other context, like cache_path, server_urls,
     timeout or other info gotten from environment variables, the
     handle data, etc. So those don't have to be reparsed and
     recreated on each request.  */
  char * winning_headers;
};

/* The cache_clean_interval_s file within the debuginfod cache specifies
   how frequently the cache should be cleaned. The file's st_mtime represents
   the time of last cleaning.  */
static const char *cache_clean_interval_filename = "cache_clean_interval_s";
static const time_t cache_clean_default_interval_s = 86400; /* 1 day */

/* The cache_miss_default_s within the debuginfod cache specifies how
   frequently the 000-permision file should be released.*/
static const time_t cache_miss_default_s = 600; /* 10 min */
static const char *cache_miss_filename = "cache_miss_s";

/* The cache_max_unused_age_s file within the debuginfod cache specifies the
   the maximum time since last access that a file will remain in the cache.  */
static const char *cache_max_unused_age_filename = "max_unused_age_s";
static const time_t cache_default_max_unused_age_s = 604800; /* 1 week */

/* Location of the cache of files downloaded from debuginfods.
   The default parent directory is $HOME, or '/' if $HOME doesn't exist.  */
static const char *cache_default_name = ".debuginfod_client_cache";
static const char *cache_xdg_name = "debuginfod_client";

/* URLs of debuginfods, separated by url_delim. */
static const char *url_delim =  " ";

/* Timeout for debuginfods, in seconds (to get at least 100K). */
static const long default_timeout = 90;

/* Default retry count for download error. */
static const long default_retry_limit = 2;

/* Data associated with a particular CURL easy handle. Passed to
   the write callback.  */
struct handle_data
{
  /* Cache file to be written to in case query is successful.  */
  int fd;

  /* URL queried by this handle.  */
  char url[PATH_MAX];

  /* error buffer for this handle.  */
  char errbuf[CURL_ERROR_SIZE];

  /* This handle.  */
  CURL *handle;

  /* The client object whom we're serving. */
  debuginfod_client *client;

  /* Pointer to handle that should write to fd. Initially points to NULL,
     then points to the first handle that begins writing the target file
     to the cache. Used to ensure that a file is not downloaded from
     multiple servers unnecessarily.  */
  CURL **target_handle;
  /* Response http headers for this client handle, sent from the server */
  char *response_data;
  size_t response_data_size;
};

static size_t
debuginfod_write_callback (char *ptr, size_t size, size_t nmemb, void *data)
{
  ssize_t count = size * nmemb;

  struct handle_data *d = (struct handle_data*)data;

  /* Indicate to other handles that they can abort their transfer.  */
  if (*d->target_handle == NULL)
    {
      *d->target_handle = d->handle;
      /* update the client object */
      const char *url = NULL;
      (void) curl_easy_getinfo (d->handle, CURLINFO_EFFECTIVE_URL, &url);
      if (url)
        {
          free (d->client->url);
          d->client->url = strdup(url); /* ok if fails */
        }
    }

  /* If this handle isn't the target handle, abort transfer.  */
  if (*d->target_handle != d->handle)
    return -1;

  return (size_t) write(d->fd, (void*)ptr, count);
}

/* handle config file read and write */
static int
debuginfod_config_cache(char *config_path,
			long cache_config_default_s,
			struct stat *st)
{
  int fd;
  /* if the config file doesn't exist, create one with DEFFILEMODE*/
  if(stat(config_path, st) == -1)
    {
      fd = open(config_path, O_CREAT | O_RDWR, DEFFILEMODE);
      if (fd < 0)
        return -errno;

      if (dprintf(fd, "%ld", cache_config_default_s) < 0)
        return -errno;
    }

  long cache_config;
  FILE *config_file = fopen(config_path, "r");
  if (config_file)
    {
      if (fscanf(config_file, "%ld", &cache_config) != 1)
        cache_config = cache_config_default_s;
      fclose(config_file);
    }
  else
    cache_config = cache_config_default_s;

  return cache_config;
}

/* Create the cache and interval file if they do not already exist.
   Return 0 if cache and config file are initialized, otherwise return
   the appropriate error code.  */
static int
debuginfod_init_cache (char *cache_path, char *interval_path, char *maxage_path)
{
  struct stat st;

  /* If the cache and config file already exist then we are done.  */
  if (stat(cache_path, &st) == 0 && stat(interval_path, &st) == 0)
    return 0;

  /* Create the cache and config files as necessary.  */
  if (stat(cache_path, &st) != 0 && mkdir(cache_path, ACCESSPERMS) < 0)
    return -errno;

  int fd = -1;

  /* init cleaning interval config file.  */
  fd = open(interval_path, O_CREAT | O_RDWR, DEFFILEMODE);
  if (fd < 0)
    return -errno;

  if (dprintf(fd, "%ld", cache_clean_default_interval_s) < 0)
    return -errno;

  /* init max age config file.  */
  if (stat(maxage_path, &st) != 0
      && (fd = open(maxage_path, O_CREAT | O_RDWR, DEFFILEMODE)) < 0)
    return -errno;

  if (dprintf(fd, "%ld", cache_default_max_unused_age_s) < 0)
    return -errno;

  return 0;
}


/* Delete any files that have been unmodied for a period
   longer than $DEBUGINFOD_CACHE_CLEAN_INTERVAL_S.  */
static int
debuginfod_clean_cache(debuginfod_client *c,
		       char *cache_path, char *interval_path,
		       char *max_unused_path)
{
  time_t clean_interval, max_unused_age;
  int rc = -1;
  struct stat st;

  /* Create new interval file.  */
  rc = debuginfod_config_cache(interval_path,
			       cache_clean_default_interval_s, &st);
  if (rc < 0)
    return rc;
  clean_interval = (time_t)rc;

  /* Check timestamp of interval file to see whether cleaning is necessary.  */
  if (time(NULL) - st.st_mtime < clean_interval)
    /* Interval has not passed, skip cleaning.  */
    return 0;

  /* Read max unused age value from config file.  */
  rc = debuginfod_config_cache(max_unused_path,
			       cache_default_max_unused_age_s, &st);
  if (rc < 0)
    return rc;
  max_unused_age = (time_t)rc;

  char * const dirs[] = { cache_path, NULL, };

  FTS *fts = fts_open(dirs, 0, NULL);
  if (fts == NULL)
    return -errno;

  regex_t re;
  const char * pattern = ".*/[a-f0-9]+(/debuginfo|/executable|/source.*|)$"; /* include dirs */
  if (regcomp (&re, pattern, REG_EXTENDED | REG_NOSUB) != 0)
    return -ENOMEM;

  FTSENT *f;
  long files = 0;
  time_t now = time(NULL);
  while ((f = fts_read(fts)) != NULL)
    {
      /* ignore any files that do not match the pattern.  */
      if (regexec (&re, f->fts_path, 0, NULL, 0) != 0)
        continue;

      files++;
      if (c->progressfn) /* inform/check progress callback */
        if ((c->progressfn) (c, files, 0))
          break;

      switch (f->fts_info)
        {
        case FTS_F:
          /* delete file if max_unused_age has been met or exceeded w.r.t. atime.  */
          if (now - f->fts_statp->st_atime >= max_unused_age)
            (void) unlink (f->fts_path);
          break;

        case FTS_DP:
          /* Remove if old & empty.  Weaken race against concurrent creation by 
             checking mtime. */
          if (now - f->fts_statp->st_mtime >= max_unused_age)
            (void) rmdir (f->fts_path);
          break;

        default:
          ;
        }
    }
  fts_close (fts);
  regfree (&re);

  /* Update timestamp representing when the cache was last cleaned.  */
  utime (interval_path, NULL);
  return 0;
}


#define MAX_BUILD_ID_BYTES 64


static void
add_default_headers(debuginfod_client *client)
{
  if (client->user_agent_set_p)
    return;

  /* Compute a User-Agent: string to send.  The more accurately this
     describes this host, the likelier that the debuginfod servers
     might be able to locate debuginfo for us. */

  char* utspart = NULL;
  struct utsname uts;
  int rc = 0;
  rc = uname (&uts);
  if (rc == 0)
    rc = asprintf(& utspart, "%s/%s", uts.sysname, uts.machine);
  if (rc < 0)
    utspart = NULL;

  FILE *f = fopen ("/etc/os-release", "r");
  if (f == NULL)
    f = fopen ("/usr/lib/os-release", "r");
  char *id = NULL;
  char *version = NULL;
  if (f != NULL)
    {
      while (id == NULL || version == NULL)
        {
          char buf[128];
          char *s = &buf[0];
          if (fgets (s, sizeof(buf), f) == NULL)
            break;

          int len = strlen (s);
          if (len < 3)
            continue;
          if (s[len - 1] == '\n')
            {
              s[len - 1] = '\0';
              len--;
            }

          char *v = strchr (s, '=');
          if (v == NULL || strlen (v) < 2)
            continue;

          /* Split var and value. */
          *v = '\0';
          v++;

          /* Remove optional quotes around value string. */
          if (*v == '"' || *v == '\'')
            {
              v++;
              s[len - 1] = '\0';
            }
          if (strcmp (s, "ID") == 0)
            id = strdup (v);
          if (strcmp (s, "VERSION_ID") == 0)
            version = strdup (v);
        }
      fclose (f);
    }

  char *ua = NULL;
  rc = asprintf(& ua, "User-Agent: %s/%s,%s,%s/%s",
                PACKAGE_NAME, PACKAGE_VERSION,
                utspart ?: "",
                id ?: "",
                version ?: "");
  if (rc < 0)
    ua = NULL;

  if (ua)
    (void) debuginfod_add_http_header (client, ua);

  free (ua);
  free (id);
  free (version);
  free (utspart);
}


#define xalloc_str(p, fmt, args...)        \
  do                                       \
    {                                      \
      if (asprintf (&p, fmt, args) < 0)    \
        {                                  \
          p = NULL;                        \
          rc = -ENOMEM;                    \
          goto out;                        \
        }                                  \
    } while (0)


/* Offer a basic form of progress tracing */
static int
default_progressfn (debuginfod_client *c, long a, long b)
{
  const char* url = debuginfod_get_url (c);
  int len = 0;

  /* We prefer to print the host part of the URL to keep the
     message short. */
  if (url != NULL)
    {
      const char* buildid = strstr(url, "buildid/");
      if (buildid != NULL)
        len = (buildid - url);
      else
        len = strlen(url);
    }

  if (b == 0 || url==NULL) /* early stage */
    dprintf(STDERR_FILENO,
            "\rDownloading %c", "-/|\\"[a % 4]);
  else if (b < 0) /* download in progress but unknown total length */
    dprintf(STDERR_FILENO,
            "\rDownloading from %.*s %ld",
            len, url, a);
  else /* download in progress, and known total length */
    dprintf(STDERR_FILENO,
            "\rDownloading from %.*s %ld/%ld",
            len, url, a, b);
  c->default_progressfn_printed_p = 1;

  return 0;
}

/* This is a callback function that receives http response headers in buffer for use
 * in this program. https://curl.se/libcurl/c/CURLOPT_HEADERFUNCTION.html is the
 * online documentation.
 */
static size_t
header_callback (char * buffer, size_t size, size_t numitems, void * userdata)
{
  if (size != 1)
    return 0;
  /* Temporary buffer for realloc */
  char *temp = NULL;
  struct handle_data *data = (struct handle_data *) userdata;
  if (data->response_data == NULL)
    {
      temp = malloc(numitems+1);
      if (temp == NULL)
        return 0;
    }
  else
    {
      temp = realloc(data->response_data, data->response_data_size + numitems + 1);
      if (temp == NULL)
        return 0;
    }

  memcpy(temp + data->response_data_size, buffer, numitems);
  data->response_data = temp;
  data->response_data_size += numitems;
  data->response_data[data->response_data_size] = '\0';
  return numitems;
}

/* Query each of the server URLs found in $DEBUGINFOD_URLS for the file
   with the specified build-id, type (debuginfo, executable or source)
   and filename. filename may be NULL. If found, return a file
   descriptor for the target, otherwise return an error code.
*/
static int
debuginfod_query_server (debuginfod_client *c,
			 const unsigned char *build_id,
                         int build_id_len,
                         const char *type,
                         const char *filename,
                         char **path)
{
  char *server_urls;
  char *urls_envvar;
  char *cache_path = NULL;
  char *maxage_path = NULL;
  char *interval_path = NULL;
  char *cache_miss_path = NULL;
  char *target_cache_dir = NULL;
  char *target_cache_path = NULL;
  char *target_cache_tmppath = NULL;
  char suffix[PATH_MAX + 1]; /* +1 for zero terminator.  */
  char build_id_bytes[MAX_BUILD_ID_BYTES * 2 + 1];
  int vfd = c->verbose_fd;
  int rc;

  if (vfd >= 0)
    {
      dprintf (vfd, "debuginfod_find_%s ", type);
      if (build_id_len == 0) /* expect clean hexadecimal */
	dprintf (vfd, "%s", (const char *) build_id);
      else
	for (int i = 0; i < build_id_len; i++)
	  dprintf (vfd, "%02x", build_id[i]);
      if (filename != NULL)
	dprintf (vfd, " %s\n", filename);
      dprintf (vfd, "\n");
    }

  /* Is there any server we can query?  If not, don't do any work,
     just return with ENOSYS.  Don't even access the cache.  */
  urls_envvar = getenv(DEBUGINFOD_URLS_ENV_VAR);
  if (vfd >= 0)
    dprintf (vfd, "server urls \"%s\"\n",
	     urls_envvar != NULL ? urls_envvar : "");
  if (urls_envvar == NULL || urls_envvar[0] == '\0')
    {
      rc = -ENOSYS;
      goto out;
    }

  /* Clear the obsolete URL from a previous _find operation. */
  free (c->url);
  c->url = NULL;

  /* PR 27982: Add max size if DEBUGINFOD_MAXSIZE is set. */
  long maxsize = 0;
  const char *maxsize_envvar;
  maxsize_envvar = getenv(DEBUGINFOD_MAXSIZE_ENV_VAR);
  if (maxsize_envvar != NULL)
    maxsize = atol (maxsize_envvar);

  /* PR 27982: Add max time if DEBUGINFOD_MAXTIME is set. */
  long maxtime = 0;
  const char *maxtime_envvar;
  maxtime_envvar = getenv(DEBUGINFOD_MAXTIME_ENV_VAR);
  if (maxtime_envvar != NULL)
    maxtime = atol (maxtime_envvar);
  if (maxtime && vfd >= 0)
    dprintf(vfd, "using max time %lds\n", maxtime);

  /* Maxsize is valid*/
  if (maxsize > 0)
    {
      if (vfd)
        dprintf (vfd, "using max size %ldB\n", maxsize);
      char *size_header = NULL;
      rc = asprintf (&size_header, "X-DEBUGINFOD-MAXSIZE: %ld", maxsize);
      if (rc < 0)
        {
          rc = -ENOMEM;
          goto out;
        }
      rc = debuginfod_add_http_header(c, size_header);
      free(size_header);
      if (rc < 0)
        goto out;
    }
  add_default_headers(c);

  /* Copy lowercase hex representation of build_id into buf.  */
  if (vfd >= 0)
    dprintf (vfd, "checking build-id\n");
  if ((build_id_len >= MAX_BUILD_ID_BYTES) ||
      (build_id_len == 0 &&
       strlen ((const char *) build_id) > MAX_BUILD_ID_BYTES*2))
    {
      rc = -EINVAL;
      goto out;
    }

  if (build_id_len == 0) /* expect clean hexadecimal */
    strcpy (build_id_bytes, (const char *) build_id);
  else
    for (int i = 0; i < build_id_len; i++)
      sprintf(build_id_bytes + (i * 2), "%02x", build_id[i]);

  if (filename != NULL)
    {
      if (vfd >= 0)
	dprintf (vfd, "checking filename\n");
      if (filename[0] != '/') // must start with /
	{
	  rc = -EINVAL;
	  goto out;
	}

      /* copy the filename to suffix, s,/,#,g */
      unsigned q = 0;
      for (unsigned fi=0; q < PATH_MAX-2; fi++) /* -2, escape is 2 chars.  */
        switch (filename[fi])
          {
          case '\0':
            suffix[q] = '\0';
            q = PATH_MAX-1; /* escape for loop too */
            break;
          case '/': /* escape / to prevent dir escape */
            suffix[q++]='#';
            suffix[q++]='#';
            break;
          case '#': /* escape # to prevent /# vs #/ collisions */
            suffix[q++]='#';
            suffix[q++]='_';
            break;
          default:
            suffix[q++]=filename[fi];
          }
      suffix[q] = '\0';
      /* If the DWARF filenames are super long, this could exceed
         PATH_MAX and truncate/collide.  Oh well, that'll teach
         them! */
    }
  else
    suffix[0] = '\0';

  if (suffix[0] != '\0' && vfd >= 0)
    dprintf (vfd, "suffix %s\n", suffix);

  /* set paths needed to perform the query

     example format
     cache_path:        $HOME/.cache
     target_cache_dir:  $HOME/.cache/0123abcd
     target_cache_path: $HOME/.cache/0123abcd/debuginfo
     target_cache_path: $HOME/.cache/0123abcd/source#PATH#TO#SOURCE ?

     $XDG_CACHE_HOME takes priority over $HOME/.cache.
     $DEBUGINFOD_CACHE_PATH takes priority over $HOME/.cache and $XDG_CACHE_HOME.
  */

  /* Determine location of the cache. The path specified by the debuginfod
     cache environment variable takes priority.  */
  char *cache_var = getenv(DEBUGINFOD_CACHE_PATH_ENV_VAR);
  if (cache_var != NULL && strlen (cache_var) > 0)
    xalloc_str (cache_path, "%s", cache_var);
  else
    {
      /* If a cache already exists in $HOME ('/' if $HOME isn't set), then use
         that. Otherwise use the XDG cache directory naming format.  */
      xalloc_str (cache_path, "%s/%s", getenv ("HOME") ?: "/", cache_default_name);

      struct stat st;
      if (stat (cache_path, &st) < 0)
        {
          char cachedir[PATH_MAX];
          char *xdg = getenv ("XDG_CACHE_HOME");

          if (xdg != NULL && strlen (xdg) > 0)
            snprintf (cachedir, PATH_MAX, "%s", xdg);
          else
            snprintf (cachedir, PATH_MAX, "%s/.cache", getenv ("HOME") ?: "/");

          /* Create XDG cache directory if it doesn't exist.  */
          if (stat (cachedir, &st) == 0)
            {
              if (! S_ISDIR (st.st_mode))
                {
                  rc = -EEXIST;
                  goto out;
                }
            }
          else
            {
              rc = mkdir (cachedir, 0700);

              /* Also check for EEXIST and S_ISDIR in case another client just
                 happened to create the cache.  */
              if (rc < 0
                  && (errno != EEXIST
                      || stat (cachedir, &st) != 0
                      || ! S_ISDIR (st.st_mode)))
                {
                  rc = -errno;
                  goto out;
                }
            }

          free (cache_path);
          xalloc_str (cache_path, "%s/%s", cachedir, cache_xdg_name);
        }
    }

  xalloc_str (target_cache_dir, "%s/%s", cache_path, build_id_bytes);
  xalloc_str (target_cache_path, "%s/%s%s", target_cache_dir, type, suffix);
  xalloc_str (target_cache_tmppath, "%s.XXXXXX", target_cache_path);

  /* XXX combine these */
  xalloc_str (interval_path, "%s/%s", cache_path, cache_clean_interval_filename);
  xalloc_str (cache_miss_path, "%s/%s", cache_path, cache_miss_filename);
  xalloc_str (maxage_path, "%s/%s", cache_path, cache_max_unused_age_filename);

  if (vfd >= 0)
    dprintf (vfd, "checking cache dir %s\n", cache_path);

  rc = debuginfod_init_cache(cache_path, interval_path, maxage_path);
  if (rc != 0)
    goto out;
  rc = debuginfod_clean_cache(c, cache_path, interval_path, maxage_path);
  if (rc != 0)
    goto out;

  struct stat st;
  /* Check if the file exists and it's of permission 000; must check
     explicitly rather than trying to open it first (PR28240). */
  if (stat(target_cache_path, &st) == 0
      && (st.st_mode & 0777) == 0)
    {
      time_t cache_miss;

      rc = debuginfod_config_cache(cache_miss_path, cache_miss_default_s, &st);
      if (rc < 0)
        goto out;

      cache_miss = (time_t)rc;
      if (time(NULL) - st.st_mtime <= cache_miss)
        {
         rc = -ENOENT;
         goto out;
       }
      else
        /* TOCTOU non-problem: if another task races, puts a working
           download or a 000 file in its place, unlinking here just
           means WE will try to download again as uncached. */
        unlink(target_cache_path);
    }
  
  /* If the target is already in the cache (known not-000 - PR28240), 
     then we are done. */
  int fd = open (target_cache_path, O_RDONLY);
  if (fd >= 0)
    {
      /* Success!!!! */
      if (path != NULL)
        *path = strdup(target_cache_path);
      rc = fd;
      goto out;
    }

  long timeout = default_timeout;
  const char* timeout_envvar = getenv(DEBUGINFOD_TIMEOUT_ENV_VAR);
  if (timeout_envvar != NULL)
    timeout = atoi (timeout_envvar);

  if (vfd >= 0)
    dprintf (vfd, "using timeout %ld\n", timeout);

  /* make a copy of the envvar so it can be safely modified.  */
  server_urls = strdup(urls_envvar);
  if (server_urls == NULL)
    {
      rc = -ENOMEM;
      goto out;
    }
  /* thereafter, goto out0 on error*/

  /* Because of a race with cache cleanup / rmdir, try to mkdir/mkstemp up to twice. */
  for(int i=0; i<2; i++) {
    /* (re)create target directory in cache */
    (void) mkdir(target_cache_dir, 0700); /* files will be 0400 later */

    /* NB: write to a temporary file first, to avoid race condition of
       multiple clients checking the cache, while a partially-written or empty
       file is in there, being written from libcurl. */
    fd = mkstemp (target_cache_tmppath);
    if (fd >= 0) break;
  }
  if (fd < 0) /* Still failed after two iterations. */
    {
      rc = -errno;
      goto out0;
    }

  /* Initialize the memory to zero */
  char *strtok_saveptr;
  char **server_url_list = NULL;
  char *server_url = strtok_r(server_urls, url_delim, &strtok_saveptr);
  /* Count number of URLs.  */
  int num_urls = 0;

  while (server_url != NULL)
    {
      /* PR 27983: If the url is already set to be used use, skip it */
      char *slashbuildid;
      if (strlen(server_url) > 1 && server_url[strlen(server_url)-1] == '/')
        slashbuildid = "buildid";
      else
        slashbuildid = "/buildid";

      char *tmp_url;
      if (asprintf(&tmp_url, "%s%s", server_url, slashbuildid) == -1)
        {
          rc = -ENOMEM;
          goto out1;
        }
      int url_index;
      for (url_index = 0; url_index < num_urls; ++url_index)
        {
          if(strcmp(tmp_url, server_url_list[url_index]) == 0)
            {
              url_index = -1;
              break;
            }
        }
      if (url_index == -1)
        {
          if (vfd >= 0)
            dprintf(vfd, "duplicate url: %s, skipping\n", tmp_url);
          free(tmp_url);
        }
      else
        {
          num_urls++;
          char ** realloc_ptr;
          realloc_ptr = reallocarray(server_url_list, num_urls,
                                         sizeof(char*));
          if (realloc_ptr == NULL)
            {
              rc = -ENOMEM;
              goto out1;
            }
          server_url_list = realloc_ptr;
          server_url_list[num_urls-1] = tmp_url;
        }
      server_url = strtok_r(NULL, url_delim, &strtok_saveptr);
    }

  int retry_limit = default_retry_limit;
  const char* retry_limit_envvar = getenv(DEBUGINFOD_RETRY_LIMIT_ENV_VAR);
  if (retry_limit_envvar != NULL)
    retry_limit = atoi (retry_limit_envvar);

  CURLM *curlm = c->server_mhandle;
  assert (curlm != NULL);

  /* Tracks which handle should write to fd. Set to the first
     handle that is ready to write the target file to the cache.  */
  CURL *target_handle = NULL;
  struct handle_data *data = malloc(sizeof(struct handle_data) * num_urls);
  if (data == NULL)
    {
      rc = -ENOMEM;
      goto out1;
    }

  /* thereafter, goto out1 on error.  */

 /*The beginning of goto block query_in_parallel.*/
 query_in_parallel:
  rc = -ENOENT; /* Reset rc to default.*/

  /* Initialize handle_data with default values. */
  for (int i = 0; i < num_urls; i++)
    {
      data[i].handle = NULL;
      data[i].fd = -1;
      data[i].errbuf[0] = '\0';
    }

  char *escaped_string = NULL;
  size_t escaped_strlen = 0;
  if (filename)
    {
      escaped_string = curl_easy_escape(&target_handle, filename+1, 0);
      if (!escaped_string)
        {
          rc = -ENOMEM;
          goto out2;
        }
      char *loc = escaped_string;
      escaped_strlen = strlen(escaped_string);
      while ((loc = strstr(loc, "%2F")))
        {
          loc[0] = '/';
          //pull the string back after replacement
          // loc-escaped_string finds the distance from the origin to the new location
          // - 2 accounts for the 2F which remain and don't need to be measured.
          // The two above subtracted from escaped_strlen yields the remaining characters
          // in the string which we want to pull back
          memmove(loc+1, loc+3,escaped_strlen - (loc-escaped_string) - 2);
          //Because the 2F was overwritten in the memmove (as desired) escaped_strlen is
          // now two shorter.
          escaped_strlen -= 2;
        }
    }
  /* Initialize each handle.  */
  for (int i = 0; i < num_urls; i++)
    {
      if ((server_url = server_url_list[i]) == NULL)
        break;
      if (vfd >= 0)
	dprintf (vfd, "init server %d %s\n", i, server_url);

      data[i].fd = fd;
      data[i].target_handle = &target_handle;
      data[i].handle = curl_easy_init();
      if (data[i].handle == NULL)
        {
          rc = -ENETUNREACH;
          goto out1;
        }
      data[i].client = c;

      if (filename) /* must start with / */
        {
          /* PR28034 escape characters in completed url to %hh format. */
          snprintf(data[i].url, PATH_MAX, "%s/%s/%s/%s", server_url,
                   build_id_bytes, type, escaped_string);
        }
      else
        snprintf(data[i].url, PATH_MAX, "%s/%s/%s", server_url, build_id_bytes, type);
      if (vfd >= 0)
	dprintf (vfd, "url %d %s\n", i, data[i].url);

      /* Only allow http:// + https:// + file:// so we aren't being
	 redirected to some unsupported protocol.  */
      curl_easy_setopt(data[i].handle, CURLOPT_PROTOCOLS,
		       CURLPROTO_HTTP | CURLPROTO_HTTPS | CURLPROTO_FILE);
      curl_easy_setopt(data[i].handle, CURLOPT_URL, data[i].url);
      if (vfd >= 0)
	curl_easy_setopt(data[i].handle, CURLOPT_ERRORBUFFER, data[i].errbuf);
      curl_easy_setopt(data[i].handle,
                       CURLOPT_WRITEFUNCTION,
                       debuginfod_write_callback);
      curl_easy_setopt(data[i].handle, CURLOPT_WRITEDATA, (void*)&data[i]);
      if (timeout > 0)
	{
	  /* Make sure there is at least some progress,
	     try to get at least 100K per timeout seconds.  */
	  curl_easy_setopt (data[i].handle, CURLOPT_LOW_SPEED_TIME,
			    timeout);
	  curl_easy_setopt (data[i].handle, CURLOPT_LOW_SPEED_LIMIT,
			    100 * 1024L);
	}
      data[i].response_data = NULL;
      data[i].response_data_size = 0;
      curl_easy_setopt(data[i].handle, CURLOPT_FILETIME, (long) 1);
      curl_easy_setopt(data[i].handle, CURLOPT_FOLLOWLOCATION, (long) 1);
      curl_easy_setopt(data[i].handle, CURLOPT_FAILONERROR, (long) 1);
      curl_easy_setopt(data[i].handle, CURLOPT_NOSIGNAL, (long) 1);
      curl_easy_setopt(data[i].handle, CURLOPT_HEADERFUNCTION, header_callback);
      curl_easy_setopt(data[i].handle, CURLOPT_HEADERDATA, (void *) &(data[i]));
#if LIBCURL_VERSION_NUM >= 0x072a00 /* 7.42.0 */
      curl_easy_setopt(data[i].handle, CURLOPT_PATH_AS_IS, (long) 1);
#else
      /* On old curl; no big deal, canonicalization here is almost the
         same, except perhaps for ? # type decorations at the tail. */
#endif
      curl_easy_setopt(data[i].handle, CURLOPT_AUTOREFERER, (long) 1);
      curl_easy_setopt(data[i].handle, CURLOPT_ACCEPT_ENCODING, "");
      curl_easy_setopt(data[i].handle, CURLOPT_HTTPHEADER, c->headers);

      curl_multi_add_handle(curlm, data[i].handle);
    }

  if (filename) curl_free(escaped_string);
  /* Query servers in parallel.  */
  if (vfd >= 0)
    dprintf (vfd, "query %d urls in parallel\n", num_urls);
  int still_running;
  long loops = 0;
  int committed_to = -1;
  bool verbose_reported = false;
  struct timespec start_time, cur_time;
  c->winning_headers = NULL;
  if ( maxtime > 0 && clock_gettime(CLOCK_MONOTONIC_RAW, &start_time) == -1)
    {
      rc = errno;
      goto out2;
    }
  long delta = 0;
  do
    {
      /* Check to see how long querying is taking. */
      if (maxtime > 0)
        {
          if (clock_gettime(CLOCK_MONOTONIC_RAW, &cur_time) == -1)
            {
              rc = errno;
              goto out2;
            }
          delta = cur_time.tv_sec - start_time.tv_sec;
          if ( delta >  maxtime)
            {
              dprintf(vfd, "Timeout with max time=%lds and transfer time=%lds\n", maxtime, delta );
              rc = -ETIME;
              goto out2;
            }
        }
      /* Wait 1 second, the minimum DEBUGINFOD_TIMEOUT.  */
      curl_multi_wait(curlm, NULL, 0, 1000, NULL);
      CURLMcode curlm_res = curl_multi_perform(curlm, &still_running);

      /* If the target file has been found, abort the other queries.  */
      if (target_handle != NULL)
	{
	  for (int i = 0; i < num_urls; i++)
	    if (data[i].handle != target_handle)
	      curl_multi_remove_handle(curlm, data[i].handle);
	    else
              {
	        committed_to = i;
                if (c->winning_headers == NULL)
                  {
                    c->winning_headers = data[committed_to].response_data;
                    if (vfd >= 0 && c->winning_headers != NULL)
                      dprintf(vfd, "\n%s", c->winning_headers);
                    data[committed_to].response_data = NULL;
                  }

              }
	}

      if (vfd >= 0 && !verbose_reported && committed_to >= 0)
	{
	  bool pnl = (c->default_progressfn_printed_p && vfd == STDERR_FILENO);
	  dprintf (vfd, "%scommitted to url %d\n", pnl ? "\n" : "",
		   committed_to);
	  if (pnl)
	    c->default_progressfn_printed_p = 0;
	  verbose_reported = true;
	}

      if (curlm_res != CURLM_OK)
        {
          switch (curlm_res)
            {
            case CURLM_CALL_MULTI_PERFORM: continue;
            case CURLM_OUT_OF_MEMORY: rc = -ENOMEM; break;
            default: rc = -ENETUNREACH; break;
            }
          goto out2;
        }

      if (c->progressfn) /* inform/check progress callback */
        {
          loops ++;
          long pa = loops; /* default params for progress callback */
          long pb = 0; /* transfer_timeout tempting, but loops != elapsed-time */
          if (target_handle) /* we've committed to a server; report its download progress */
            {
              CURLcode curl_res;
#ifdef CURLINFO_SIZE_DOWNLOAD_T
              curl_off_t dl;
              curl_res = curl_easy_getinfo(target_handle,
                                           CURLINFO_SIZE_DOWNLOAD_T,
                                           &dl);
              if (curl_res == 0 && dl >= 0)
                pa = (dl > LONG_MAX ? LONG_MAX : (long)dl);
#else
              double dl;
              curl_res = curl_easy_getinfo(target_handle,
                                           CURLINFO_SIZE_DOWNLOAD,
                                           &dl);
              if (curl_res == 0)
                pa = (dl >= (double)(LONG_MAX+1UL) ? LONG_MAX : (long)dl);
#endif

              /* NB: If going through deflate-compressing proxies, this
                 number is likely to be unavailable, so -1 may show. */
#ifdef CURLINFO_CONTENT_LENGTH_DOWNLOAD_T
              curl_off_t cl;
              curl_res = curl_easy_getinfo(target_handle,
                                           CURLINFO_CONTENT_LENGTH_DOWNLOAD_T,
                                           &cl);
              if (curl_res == 0 && cl >= 0)
                pb = (cl > LONG_MAX ? LONG_MAX : (long)cl);
#else
              double cl;
              curl_res = curl_easy_getinfo(target_handle,
                                           CURLINFO_CONTENT_LENGTH_DOWNLOAD,
                                           &cl);
              if (curl_res == 0)
                pb = (cl >= (double)(LONG_MAX+1UL) ? LONG_MAX : (long)cl);
#endif
            }

          if ((*c->progressfn) (c, pa, pb))
            break;
        }
      /* Check to see if we are downloading something which exceeds maxsize, if set.*/
      if (maxsize > 0 && target_handle)
        {
          long dl_size = 0;
#ifdef CURLINFO_SIZE_DOWNLOAD_T
          curl_off_t download_size_t;
          if (curl_easy_getinfo(target_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T,
                                                    &download_size_t) == CURLE_OK)
            dl_size = download_size_t >= (double)(LONG_MAX+1UL) ? LONG_MAX : (long)download_size_t;
#else
          double download_size;
          if (curl_easy_getinfo(target_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD,
                                                    &download_size) == CURLE_OK)
            dl_size = download_size >= (double)(LONG_MAX+1UL) ? LONG_MAX : (long)download_size;
#endif
            if (dl_size > maxsize)
              {
                if (vfd >=0)
                  dprintf(vfd, "Content-Length too large.\n");
                rc = -EFBIG;
                goto out2;
              }
        }
    } while (still_running);

  /* Check whether a query was successful. If so, assign its handle
     to verified_handle.  */
  int num_msg;
  rc = -ENOENT;
  CURL *verified_handle = NULL;
  do
    {
      CURLMsg *msg;

      msg = curl_multi_info_read(curlm, &num_msg);
      if (msg != NULL && msg->msg == CURLMSG_DONE)
        {
	  if (vfd >= 0)
	    {
	      bool pnl = (c->default_progressfn_printed_p
			  && vfd == STDERR_FILENO);
	      dprintf (vfd, "%sserver response %s\n", pnl ? "\n" : "",
		       curl_easy_strerror (msg->data.result));
	      if (pnl)
		c->default_progressfn_printed_p = 0;
	      for (int i = 0; i < num_urls; i++)
		if (msg->easy_handle == data[i].handle)
		  {
		    if (strlen (data[i].errbuf) > 0)
		      dprintf (vfd, "url %d %s\n", i, data[i].errbuf);
		    break;
		  }
	    }

          if (msg->data.result != CURLE_OK)
            {
              long resp_code;
              CURLcode ok0;
              /* Unsuccessful query, determine error code.  */
              switch (msg->data.result)
                {
                case CURLE_COULDNT_RESOLVE_HOST: rc = -EHOSTUNREACH; break; // no NXDOMAIN
                case CURLE_URL_MALFORMAT: rc = -EINVAL; break;
                case CURLE_COULDNT_CONNECT: rc = -ECONNREFUSED; break;
                case CURLE_PEER_FAILED_VERIFICATION: rc = -ECONNREFUSED; break;
                case CURLE_REMOTE_ACCESS_DENIED: rc = -EACCES; break;
                case CURLE_WRITE_ERROR: rc = -EIO; break;
                case CURLE_OUT_OF_MEMORY: rc = -ENOMEM; break;
                case CURLE_TOO_MANY_REDIRECTS: rc = -EMLINK; break;
                case CURLE_SEND_ERROR: rc = -ECONNRESET; break;
                case CURLE_RECV_ERROR: rc = -ECONNRESET; break;
                case CURLE_OPERATION_TIMEDOUT: rc = -ETIME; break;
                case CURLE_HTTP_RETURNED_ERROR:
                  ok0 = curl_easy_getinfo (msg->easy_handle,
                                          CURLINFO_RESPONSE_CODE,
				          &resp_code);
                  /* 406 signals that the requested file was too large */
                  if ( ok0 == CURLE_OK && resp_code == 406)
                    rc = -EFBIG;
                  else
                    rc = -ENOENT;
                  break;
                default: rc = -ENOENT; break;
                }
            }
          else
            {
              /* Query completed without an error. Confirm that the
                 response code is 200 when using HTTP/HTTPS and 0 when
                 using file:// and set verified_handle.  */

              if (msg->easy_handle != NULL)
                {
                  char *effective_url = NULL;
                  long resp_code = 500;
                  CURLcode ok1 = curl_easy_getinfo (target_handle,
						    CURLINFO_EFFECTIVE_URL,
						    &effective_url);
                  CURLcode ok2 = curl_easy_getinfo (target_handle,
						    CURLINFO_RESPONSE_CODE,
						    &resp_code);
                  if(ok1 == CURLE_OK && ok2 == CURLE_OK && effective_url)
                    {
                      if (strncasecmp (effective_url, "HTTP", 4) == 0)
                        if (resp_code == 200)
                          {
                            verified_handle = msg->easy_handle;
                            break;
                          }
                      if (strncasecmp (effective_url, "FILE", 4) == 0)
                        if (resp_code == 0)
                          {
                            verified_handle = msg->easy_handle;
                            break;
                          }
                    }
                  /* - libcurl since 7.52.0 version start to support
                       CURLINFO_SCHEME;
                     - before 7.61.0, effective_url would give us a
                       url with upper case SCHEME added in the front;
                     - effective_url between 7.61 and 7.69 can be lack
                       of scheme if the original url doesn't include one;
                     - since version 7.69 effective_url will be provide
                       a scheme in lower case.  */
                  #if LIBCURL_VERSION_NUM >= 0x073d00 /* 7.61.0 */
                  #if LIBCURL_VERSION_NUM <= 0x074500 /* 7.69.0 */
                  char *scheme = NULL;
                  CURLcode ok3 = curl_easy_getinfo (target_handle,
                                                    CURLINFO_SCHEME,
                                                    &scheme);
                  if(ok3 == CURLE_OK && scheme)
                    {
                      if (startswith (scheme, "HTTP"))
                        if (resp_code == 200)
                          {
                            verified_handle = msg->easy_handle;
                            break;
                          }
                    }
                  #endif
                  #endif
                }
            }
        }
    } while (num_msg > 0);

  /* Create a 000-permission file named as $HOME/.cache if the query
     fails with ENOENT.*/
  if (rc == -ENOENT)
    {
      int efd = open (target_cache_path, O_CREAT|O_EXCL, 0000);
      if (efd >= 0)
        close(efd);
    }
  else if (rc == -EFBIG)
    goto out2;

  /* If the verified_handle is NULL and rc != -ENOENT, the query fails with
   * an error code other than 404, then do several retry within the retry_limit.
   * Clean up all old handles and jump back to the beginning of query_in_parallel,
   * reinitialize handles and query again.*/
  if (verified_handle == NULL)
    {
      if (rc != -ENOENT && retry_limit-- > 0)
        {
	  if (vfd >= 0)
            dprintf (vfd, "Retry failed query, %d attempt(s) remaining\n", retry_limit);
	  /* remove all handles from multi */
          for (int i = 0; i < num_urls; i++)
            {
              curl_multi_remove_handle(curlm, data[i].handle); /* ok to repeat */
              curl_easy_cleanup (data[i].handle);
              free(data[i].response_data);
            }
            free(c->winning_headers);
            c->winning_headers = NULL;
	    goto query_in_parallel;
	}
      else
	goto out2;
    }

  if (vfd >= 0)
    {
      bool pnl = c->default_progressfn_printed_p && vfd == STDERR_FILENO;
      dprintf (vfd, "%sgot file from server\n", pnl ? "\n" : "");
      if (pnl)
	c->default_progressfn_printed_p = 0;
    }

  /* we've got one!!!! */
  time_t mtime;
  CURLcode curl_res = curl_easy_getinfo(verified_handle, CURLINFO_FILETIME, (void*) &mtime);
  if (curl_res != CURLE_OK)
    mtime = time(NULL); /* fall back to current time */

  struct timeval tvs[2];
  tvs[0].tv_sec = tvs[1].tv_sec = mtime;
  tvs[0].tv_usec = tvs[1].tv_usec = 0;
  (void) futimes (fd, tvs);  /* best effort */

  /* PR27571: make cache files casually unwriteable; dirs are already 0700 */
  (void) fchmod(fd, 0400);
                
  /* rename tmp->real */
  rc = rename (target_cache_tmppath, target_cache_path);
  if (rc < 0)
    {
      rc = -errno;
      goto out2;
      /* Perhaps we need not give up right away; could retry or something ... */
    }

  /* remove all handles from multi */
  for (int i = 0; i < num_urls; i++)
    {
      curl_multi_remove_handle(curlm, data[i].handle); /* ok to repeat */
      curl_easy_cleanup (data[i].handle);
      free (data[i].response_data);
    }

  for (int i = 0; i < num_urls; ++i)
    free(server_url_list[i]);
  free(server_url_list);
  free (data);
  free (server_urls);

  /* don't close fd - we're returning it */
  /* don't unlink the tmppath; it's already been renamed. */
  if (path != NULL)
   *path = strdup(target_cache_path);

  rc = fd;
  goto out;

/* error exits */
 out2:
  /* remove all handles from multi */
  for (int i = 0; i < num_urls; i++)
    {
      curl_multi_remove_handle(curlm, data[i].handle); /* ok to repeat */
      curl_easy_cleanup (data[i].handle);
      free (data[i].response_data);
    }

  unlink (target_cache_tmppath);
  close (fd); /* before the rmdir, otherwise it'll fail */
  (void) rmdir (target_cache_dir); /* nop if not empty */
  free(data);

 out1:
  for (int i = 0; i < num_urls; ++i)
    free(server_url_list[i]);
  free(server_url_list);

 out0:
  free (server_urls);

/* general purpose exit */
 out:
  /* Reset sent headers */
  curl_slist_free_all (c->headers);
  c->headers = NULL;
  c->user_agent_set_p = 0;
  
  /* Conclude the last \r status line */
  /* Another possibility is to use the ANSI CSI n K EL "Erase in Line"
     code.  That way, the previously printed messages would be erased,
     and without a newline. */
  if (c->default_progressfn_printed_p)
    dprintf(STDERR_FILENO, "\n");

  if (vfd >= 0)
    {
      if (rc < 0)
	dprintf (vfd, "not found %s (err=%d)\n", strerror (-rc), rc);
      else
	dprintf (vfd, "found %s (fd=%d)\n", target_cache_path, rc);
    }

  free (cache_path);
  free (maxage_path);
  free (interval_path);
  free (cache_miss_path);
  free (target_cache_dir);
  free (target_cache_path);
  free (target_cache_tmppath);
  return rc;
}



/* See debuginfod.h  */
debuginfod_client  *
debuginfod_begin (void)
{
  debuginfod_client *client;
  size_t size = sizeof (struct debuginfod_client);
  client = calloc (1, size);

  if (client != NULL)
    {
      if (getenv(DEBUGINFOD_PROGRESS_ENV_VAR))
	client->progressfn = default_progressfn;
      if (getenv(DEBUGINFOD_VERBOSE_ENV_VAR))
	client->verbose_fd = STDERR_FILENO;
      else
	client->verbose_fd = -1;

      // allocate 1 curl multi handle
      client->server_mhandle = curl_multi_init ();
      if (client->server_mhandle == NULL)
	goto out1;
    }

  // extra future initialization
  
  goto out;

 out1:
  free (client);
  client = NULL;

 out:  
  return client;
}

void
debuginfod_set_user_data(debuginfod_client *client,
                         void *data)
{
  client->user_data = data;
}

void *
debuginfod_get_user_data(debuginfod_client *client)
{
  return client->user_data;
}

const char *
debuginfod_get_url(debuginfod_client *client)
{
  return client->url;
}

void
debuginfod_end (debuginfod_client *client)
{
  if (client == NULL)
    return;

  curl_multi_cleanup (client->server_mhandle);
  curl_slist_free_all (client->headers);
  free (client->winning_headers);
  free (client->url);
  free (client);
}

int
debuginfod_find_debuginfo (debuginfod_client *client,
			   const unsigned char *build_id, int build_id_len,
                           char **path)
{
  return debuginfod_query_server(client, build_id, build_id_len,
                                 "debuginfo", NULL, path);
}


/* See debuginfod.h  */
int
debuginfod_find_executable(debuginfod_client *client,
			   const unsigned char *build_id, int build_id_len,
                           char **path)
{
  return debuginfod_query_server(client, build_id, build_id_len,
                                 "executable", NULL, path);
}

/* See debuginfod.h  */
int debuginfod_find_source(debuginfod_client *client,
			   const unsigned char *build_id, int build_id_len,
                           const char *filename, char **path)
{
  return debuginfod_query_server(client, build_id, build_id_len,
                                 "source", filename, path);
}


/* Add an outgoing HTTP header.  */
int debuginfod_add_http_header (debuginfod_client *client, const char* header)
{
  /* Sanity check header value is of the form Header: Value.
     It should contain exactly one colon that isn't the first or
     last character.  */
  char *colon = strchr (header, ':');
  if (colon == NULL
      || colon == header
      || *(colon + 1) == '\0'
      || strchr (colon + 1, ':') != NULL)
    return -EINVAL;

  struct curl_slist *temp = curl_slist_append (client->headers, header);
  if (temp == NULL)
    return -ENOMEM;

  /* Track if User-Agent: is being set.  If so, signal not to add the
     default one. */
  if (startswith (header, "User-Agent:"))
    client->user_agent_set_p = 1;

  client->headers = temp;
  return 0;
}


void
debuginfod_set_progressfn(debuginfod_client *client,
			  debuginfod_progressfn_t fn)
{
  client->progressfn = fn;
}

void
debuginfod_set_verbose_fd(debuginfod_client *client, int fd)
{
  client->verbose_fd = fd;
}


/* NB: these are thread-unsafe. */
__attribute__((constructor)) attribute_hidden void libdebuginfod_ctor(void)
{
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

/* NB: this is very thread-unsafe: it breaks other threads that are still in libcurl */
__attribute__((destructor)) attribute_hidden void libdebuginfod_dtor(void)
{
  /* ... so don't do this: */
  /* curl_global_cleanup(); */
}

#endif /* DUMMY_LIBDEBUGINFOD */
