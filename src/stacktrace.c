/* Process a stream of stack samples into stack traces.
   Copyright (C) 2023-2024 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   This file incorporates work covered by the following copyright and
   permission notice:

   Copyright 2016-2019 Christian Hergert <chergert@redhat.com>

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   Subject to the terms and conditions of this license, each copyright holder
   and contributor hereby grants to those receiving rights under this license
   a perpetual, worldwide, non-exclusive, no-charge, royalty-free,
   irrevocable (except for failure to satisfy the conditions of this license)
   patent license to make, have made, use, offer to sell, sell, import, and
   otherwise transfer this software, where such license applies only to those
   patent claims, already acquired or hereafter acquired, licensable by such
   copyright holder or contributor that are necessarily infringed by:

   (a) their Contribution(s) (the licensed copyrights of copyright holders
       and non-copyrightable additions of contributors, in source or binary
       form) alone; or

   (b) combination of their Contribution(s) with the work of authorship to
       which such Contribution(s) was added by such copyright holder or
       contributor, if, at the time the Contribution is added, such addition
       causes such combination to be necessarily infringed. The patent license
       shall not apply to any other combinations which include the
       Contribution.

   Except as expressly stated above, no rights or licenses from any copyright
   holder or contributor is granted under this license, whether expressly, by
   implication, estoppel or otherwise.

   DISCLAIMER

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include <config.h>
#include <assert.h>
#include <argp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <locale.h>

#include <system.h>

#include <linux/perf_event.h>

/* TODO: Need to generalize the code beyond x86 architectures. */
#include <asm/perf_regs.h>
#ifndef _ASM_X86_PERF_REGS_H
#error "eu-stacktrace is currently limited to x86 architectures"
#endif

/*************************************
 * Includes: libdwfl data structures *
 *************************************/

/* #include ELFUTILS_HEADER(dwfl) */
#include "../libdwfl/libdwflP.h"
/* XXX: Private header needed for find_procfile, sysprof_init_dwfl */

/*************************************
 * Includes: sysprof data structures *
 *************************************/

#if HAVE_SYSPROF_6_HEADERS
#include <sysprof-6/sysprof-capture-types.h>
#define HAVE_SYSPROF_HEADERS 1
#elif HAVE_SYSPROF_4_HEADERS
#include <sysprof-4/sysprof-capture-types.h>
#define HAVE_SYSPROF_HEADERS 1
#else
#define HAVE_SYSPROF_HEADERS 0
#endif

#if HAVE_SYSPROF_HEADERS

/* XXX: To be added to new versions of sysprof.  If a
   sysprof-capture-types.h with new capture frame is being used, this
   #if should guard against duplicate declarations. */
#if SYSPROF_CAPTURE_FRAME_LAST < 19

#undef SYSPROF_CAPTURE_FRAME_LAST
#define SYSPROF_CAPTURE_FRAME_STACK_USER 18
#define SYSPROF_CAPTURE_FRAME_LAST 19

SYSPROF_ALIGNED_BEGIN(1)
typedef struct
{
  SysprofCaptureFrame   frame;
  uint64_t              size;
  int32_t               tid;
  uint32_t              padding;
  uint8_t               data[0];
} SysprofCaptureStackUser
SYSPROF_ALIGNED_END(1);

/* Does not appear standalone; instead, appended to the end of a SysprofCaptureStackUser frame. */
SYSPROF_ALIGNED_BEGIN(1)
typedef struct
{
  uint32_t              n_regs;
  uint32_t              abi;
  uint64_t              regs[0];
} SysprofCaptureUserRegs
SYSPROF_ALIGNED_END(1);

#endif /* SYSPROF_CAPTURE_FRAME_STACK_USER */

#endif /* HAVE_SYSPROF_HEADERS */

/**************************
 * Global data structures *
 **************************/

/* TODO: Reduce repeated error messages in show_failures. */

static int maxframes = 256;

static char *input_path = NULL;
static int input_fd = -1;
static char *output_path = NULL;
static int output_fd = -1;

static int signal_count = 0;

#define MODE_OPTS "none/passthru/naive"
#define MODE_NONE 0x0
#define MODE_PASSTHRU 0x1
#define MODE_NAIVE 0x2
static int processing_mode = MODE_NAIVE;

#define FORMAT_OPTS "sysprof"
#define FORMAT_PERF 0x1
#define FORMAT_SYSPROF 0x2
static int input_format;
static int output_format = FORMAT_SYSPROF;

/* non-printable argp options.  */
#define OPT_DEBUG	0x100

/* Diagnostic options. */
static bool show_memory_reads = false;
static bool show_frames = false;
static bool show_samples = false;
static bool show_failures = false;
static bool show_summary = true;

/* Environment vars to drive diagnostic options: */
#define ELFUTILS_STACKTRACE_VERBOSE_ENV_VAR "ELFUTILS_STACKTRACE_VERBOSE"
/* Valid values that turn on diagnostics: 'true', 'verbose', 'debug', '1', '2'. */

/* Enables detailed tracing of simulated memory reads: */
/* #define DEBUG_MEMORY_READ */

/* Enables even more diagnostics on modules: */
/* #define DEBUG_MODULES */

/* Enables standard access to DWARF debuginfo, matching stack.c.
   This is of dubious benefit -- for profiling, we really should
   aim to resolve everything with minimal overhead using eh CFI. */
/* #define FIND_DEBUGINFO */

/* Program exit codes.  All samples processed without any errors is
   GOOD.  Some non-fatal errors during processing is an ERROR.  A
   fatal error or no samples processed at all is BAD.  A command line
   USAGE exit is generated by argp_error. */
#define EXIT_OK     0
#define EXIT_ERROR  1
#define EXIT_BAD    2
#define EXIT_USAGE 64

/**************************
 * Sysprof format support *
 **************************/

/* TODO: elfutils (internal) libraries use libNN_set_errno and _E_WHATEVER;
   this code sets errno variable directly and uses standard EWHATEVER. */

/* XXX based on sysprof src/libsysprof-capture/sysprof-capture-reader.c

   Note: BSD license attribution at the top of the file applies to this
   segment. Could split into a separate file or even a library,
   in which case the attribution notice will move along with it. */

#if HAVE_SYSPROF_HEADERS

/* A complete passthrough can be implemented based on the following 7 functions:
 - sysprof_reader_begin/sysprof_reader_end :: sysprof_capture_reader_new_from_fd
 - sysprof_reader_getheader :: sysprof_capture_reader_read_file_header
 - sysprof_reader_getframes :: sysprof_capture_reader_discover_end_time, an example main loop that doesn't handle every type of frame
 - sysprof_reader_next_frame :: sysprof_capture_reader_peek_frame + sysprof_capture_reader_skip + sysprof_capture_reader_read_basic
 - sysprof_reader_ensure_space_for :: sysprof_capture_reader_ensure_space_for
 - sysprof_reader_bswap_frame :: sysprof_capture_reader_bswap_frame
 */

/* Callback results */
enum
{
  SYSPROF_CB_OK = 0,
  SYSPROF_CB_ABORT
};

typedef struct
{
  uint8_t *buf;
  size_t bufsz;
  size_t len;
  size_t pos;
  size_t fd_off; /* XXX track offset for debugging only */
  int fd;
  int endian;
  SysprofCaptureFileHeader header;
} SysprofReader;

/* forward decls */
SysprofReader *sysprof_reader_begin (int fd);
void sysprof_reader_end (SysprofReader *reader);
bool sysprof_reader_getheader (SysprofReader *reader,
			       SysprofCaptureFileHeader *header);
void sysprof_reader_bswap_frame (SysprofReader *reader,
				 SysprofCaptureFrame *frame);
bool sysprof_reader_ensure_space_for (SysprofReader *reader, size_t len);
SysprofCaptureFrame *sysprof_reader_next_frame (SysprofReader *reader);
ptrdiff_t sysprof_reader_getframes (SysprofReader *reader,
				    int (*callback) (SysprofCaptureFrame *frame,
						     void *arg),
				    void *arg);

SysprofReader *
sysprof_reader_begin (int fd)
{
  SysprofReader *reader;

  assert (fd > -1);

  reader = malloc (sizeof (SysprofReader));
  if (reader == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }

  reader->bufsz = USHRT_MAX * 2;
  reader->buf = malloc (reader->bufsz);
  if (reader->buf == NULL)
    {
      free (reader);
      errno = ENOMEM;
      return NULL;
    }

  reader->len = 0;
  reader->pos = 0;
  reader->fd = fd;
  reader->fd_off = 0;

  if (!sysprof_reader_getheader (reader, &reader->header))
    {
      int errsv = errno;
      sysprof_reader_end (reader);
      errno = errsv;
      return NULL;
    }

  if (reader->header.little_endian)
    reader->endian = __LITTLE_ENDIAN;
  else
    reader->endian = __BIG_ENDIAN;

  return reader;
}

void
sysprof_reader_end (SysprofReader *reader)
{
  if (reader != NULL)
    {
      free (reader->buf);
      free (reader);
    }
}

bool
sysprof_reader_getheader (SysprofReader *reader,
			  SysprofCaptureFileHeader *header)
{
  assert (reader != NULL);
  assert (header != NULL);

  if (sizeof *header != read (reader->fd, header, sizeof *header))
    {
      /* errno is propagated */
      return false;
    }
  reader->fd_off += sizeof *header;

  if (header->magic != SYSPROF_CAPTURE_MAGIC)
    {
      errno = EBADMSG;
      return false;
    }

  header->capture_time[sizeof header->capture_time - 1] = '\0';

  return true;
}

void
sysprof_reader_bswap_frame (SysprofReader *reader, SysprofCaptureFrame *frame)
{
  assert (reader != NULL);
  assert (frame  != NULL);

  if (unlikely (reader->endian != __BYTE_ORDER))
    {
      frame->len = bswap_16 (frame->len);
      frame->cpu = bswap_16 (frame->cpu);
      frame->pid = bswap_32 (frame->pid);
      frame->time = bswap_64 (frame->time);
    }
}

bool
sysprof_reader_ensure_space_for (SysprofReader *reader, size_t len)
{
  assert (reader != NULL);
  assert (reader->pos <= reader->len);
  assert (len > 0);

  /* Ensure alignment of length to read */
  len = (len + SYSPROF_CAPTURE_ALIGN - 1) & ~(SYSPROF_CAPTURE_ALIGN - 1);

  if ((reader->len - reader->pos) < len)
    {
      ssize_t r;

      if (reader->len > reader->pos)
	memmove (reader->buf,
		 &reader->buf[reader->pos],
		 reader->len - reader->pos);
      reader->len -= reader->pos;
      reader->pos = 0;

      while (reader->len < len)
	{
	  assert ((reader->pos + reader->len) < reader->bufsz);
	  assert (reader->len < reader->bufsz);

	  /* Read into our buffer */
	  r = read (reader->fd,
		    &reader->buf[reader->len],
		    reader->bufsz - reader->len);

	  if (r <= 0)
	    break;

	  reader->fd_off += r;
	  reader->len += r;
	}
    }

  return (reader->len - reader->pos) >= len;
}

/* XXX May want to signal errors in more detail with an rc. */
SysprofCaptureFrame *
sysprof_reader_next_frame (SysprofReader *reader)
{
  SysprofCaptureFrame frame_hdr;
  SysprofCaptureFrame *frame = NULL;

  assert (reader != NULL);
  assert ((reader->pos % SYSPROF_CAPTURE_ALIGN) == 0);
  assert (reader->pos <= reader->len);
  assert (reader->pos <= reader->bufsz);

  if (!sysprof_reader_ensure_space_for (reader, sizeof *frame))
    return NULL;

  assert ((reader->pos % SYSPROF_CAPTURE_ALIGN) == 0);

  frame = (SysprofCaptureFrame *)(void *)&reader->buf[reader->pos];
  frame_hdr = *frame;
  sysprof_reader_bswap_frame (reader, &frame_hdr);

  if (frame_hdr.len < sizeof (SysprofCaptureFrame))
    return NULL;

  if (!sysprof_reader_ensure_space_for (reader, frame_hdr.len))
    return NULL;

  frame = (SysprofCaptureFrame *)(void *)&reader->buf[reader->pos];
  sysprof_reader_bswap_frame (reader, frame);

  if (frame->len > (reader->len - reader->pos))
    return NULL;

  reader->pos += frame->len;

  if ((reader->pos % SYSPROF_CAPTURE_ALIGN) != 0)
    return NULL;

  /* if (frame->type < 0 || frame->type >= SYSPROF_CAPTURE_FRAME_LAST) */
  if (frame->type >= SYSPROF_CAPTURE_FRAME_LAST)
    return NULL;

  return frame;
}

ptrdiff_t
sysprof_reader_getframes (SysprofReader *reader,
			  int (*callback) (SysprofCaptureFrame *,
					   void *),
			  void *arg)
{
  SysprofCaptureFrame *frame;

  assert (reader != NULL);

  while ((frame = sysprof_reader_next_frame (reader)))
    {
      int ok = callback (frame, arg);
      if (ok != SYSPROF_CB_OK)
	return -1;
    }
  return 0;
}

#endif /* HAVE_SYSPROF_HEADERS */

/*******************************************
 * Memory read interface for stack samples *
 *******************************************/

/* TODO: elfutils (internal) libraries use libNN_set_errno and DWFL_E_WHATEVER;
   this code fails silently in sample_getthread. */

struct __sample_arg
{
  int tid;
  Dwarf_Addr base_addr;
  uint64_t size;
  uint8_t *data;
  uint64_t n_regs;
  uint64_t abi; /* PERF_SAMPLE_REGS_ABI_{32,64} */
  Dwarf_Addr pc;
  Dwarf_Addr sp;
  Dwarf_Addr *regs;
};

/* The next few functions imitate the corefile interface for a single
   stack sample, with very restricted access to registers and memory. */

/* Just yield the single thread id matching the sample. */
static pid_t
sample_next_thread (Dwfl *dwfl __attribute__ ((unused)), void *dwfl_arg,
		    void **thread_argp)
{
  struct __sample_arg *sample_arg = (struct __sample_arg *)dwfl_arg;
  if (*thread_argp == NULL)
    {
      *thread_argp = (void *)0xea7b3375;
      return sample_arg->tid;
    }
  else
    return 0;
}

/* Just check that the thread id matches the sample. */
static bool
sample_getthread (Dwfl *dwfl __attribute__ ((unused)), pid_t tid,
		  void *dwfl_arg, void **thread_argp)
{
  struct __sample_arg *sample_arg = (struct __sample_arg *)dwfl_arg;
  *thread_argp = (void *)sample_arg;
  if (sample_arg->tid != tid)
    {
      return false;
    }
  return true;
}

#define copy_word_64(result, d) \
  if ((((uintptr_t) (d)) & (sizeof (uint64_t) - 1)) == 0) \
    *(result) = *(uint64_t *)(d); \
  else \
    memcpy ((result), (d), sizeof (uint64_t));

#define copy_word_32(result, d) \
  if ((((uintptr_t) (d)) & (sizeof (uint32_t) - 1)) == 0) \
    *(result) = *(uint32_t *)(d); \
  else \
    memcpy ((result), (d), sizeof (uint32_t));

#define copy_word(result, d, abi) \
  if ((abi) == PERF_SAMPLE_REGS_ABI_64)	\
    { copy_word_64((result), (d)); } \
  else if ((abi) == PERF_SAMPLE_REGS_ABI_32) \
    { copy_word_32((result), (d)); } \
  else \
    *(result) = 0;

static bool
elf_memory_read (Dwfl *dwfl, Dwarf_Addr addr, Dwarf_Word *result, void *arg)
{
  struct __sample_arg *sample_arg = (struct __sample_arg *)arg;
  Dwfl_Module *mod = dwfl_addrmodule(dwfl, addr);
  Dwarf_Addr bias;
  Elf_Scn *section = dwfl_module_address_section(mod, &addr, &bias);

  if (!section)
    return false;

  Elf_Data *data = elf_getdata(section, NULL);
  if (data && data->d_buf && data->d_size > addr) {
    uint8_t *d = ((uint8_t *)data->d_buf) + addr;
    copy_word(result, d, sample_arg->abi);
    return true;
  }
  return false;
}

static bool
sample_memory_read (Dwfl *dwfl, Dwarf_Addr addr, Dwarf_Word *result, void *arg)
{
  struct __sample_arg *sample_arg = (struct __sample_arg *)arg;
  if (show_memory_reads)
    fprintf(stderr, "* memory_read addr=%lx+(%lx) size=%lx\n",
	    sample_arg->base_addr, addr - sample_arg->base_addr, sample_arg->size);
  /* Imitate read_cached_memory() with the stack sample data as the cache. */
  if (addr < sample_arg->base_addr || addr - sample_arg->base_addr >= sample_arg->size)
    return elf_memory_read(dwfl, addr, result, arg);
  uint8_t *d = &sample_arg->data[addr - sample_arg->base_addr];
  copy_word(result, d, sample_arg->abi);
  return true;
}

/* TODO: Need to generalize this code beyond x86 architectures. */
static bool
sample_set_initial_registers (Dwfl_Thread *thread, void *thread_arg)
{
  /* The following facts are needed to translate x86 registers correctly:
     - perf register order seen in linux arch/x86/include/uapi/asm/perf_regs.h
       The registers array is built in the same order as the enum!
       (See the code in tools/perf/util/intel-pt.c intel_pt_add_gp_regs().)
     - sysprof libsysprof/perf-event-stream-private.h records all registers
       except segment and flags.
       - TODO: Should include the perf regs mask in sysprof data and
         translate registers in fully-general fashion, removing this assumption.
     - dwarf register order seen in elfutils backends/{x86_64,i386}_initreg.c;
       and it's a fairly different register order!

     For comparison, you can study codereview.qt-project.org/gitweb?p=qt-creator/perfparser.git;a=blob;f=app/perfregisterinfo.cpp;hb=HEAD
     and follow the code which uses those tables of magic numbers.
     But it's better to follow original sources of truth for this. */
  struct __sample_arg *sample_arg = (struct __sample_arg *)thread_arg;
  bool is_abi32 = (sample_arg->abi == PERF_SAMPLE_REGS_ABI_32);
  static const int regs_i386[] = {0, 2, 3, 1, 7/*sp*/, 6, 4, 5, 8/*ip*/};
  static const int regs_x86_64[] = {0, 3, 2, 1, 4, 5, 6, 7/*sp*/, 9, 10, 11, 12, 13, 14, 15, 16, 8/*ip*/};
  const int *reg_xlat = is_abi32 ? regs_i386 : regs_x86_64;
  int n_regs = is_abi32 ? 9 : 17;
  dwfl_thread_state_register_pc (thread, sample_arg->pc);
  if (sample_arg->n_regs < (uint64_t)n_regs && show_failures)
    fprintf(stderr, N_("sample_set_initial_regs: n_regs=%ld, expected %d\n"),
	    sample_arg->n_regs, n_regs);
  for (int i = 0; i < n_regs; i++)
    {
      int j = reg_xlat[i];
      if (j < 0) continue;
      if (sample_arg->n_regs <= (uint64_t)j) continue;
      dwfl_thread_state_registers (thread, i, 1, &sample_arg->regs[j]);
    }
  return true;
}

static void
sample_detach (Dwfl *dwfl __attribute__ ((unused)), void *dwfl_arg)
{
  struct __sample_arg *sample_arg = (struct __sample_arg *)dwfl_arg;
  free (sample_arg);
}

static const Dwfl_Thread_Callbacks sample_thread_callbacks =
{
  sample_next_thread,
  sample_getthread,
  sample_memory_read,
  sample_set_initial_registers,
  sample_detach,
  NULL, /* sample_thread_detach */
};

/****************************************************
 * Dwfl and statistics table for multiple processes *
 ****************************************************/

/* This echoes lib/dynamicsizehash.* with some necessary modifications. */
typedef struct
{
  bool used;
  pid_t pid;
  Dwfl *dwfl;
  char *comm;
  int max_frames; /* for diagnostic purposes */
  int total_samples; /* for diagnostic purposes */
  int lost_samples; /* for diagnostic purposes */
  Dwfl_Unwound_Source last_unwound; /* track CFI source, for diagnostic purposes */
  Dwfl_Unwound_Source worst_unwound; /* track CFI source, for diagnostic purposes */
} dwfltab_ent;

typedef struct
{
  ssize_t size;
  ssize_t filled;
  dwfltab_ent *table;
} dwfltab;

/* XXX table size must be a prime */
#define DWFLTAB_DEFAULT_SIZE 1021
extern size_t next_prime (size_t); /* XXX from libeu.a lib/next_prime.c */
dwfltab_ent *dwfltab_find(pid_t pid); /* forward decl */

/* TODO: Could turn this into a field of sui instead of a global. */
dwfltab default_table;

/* XXX based on lib/dynamicsizehash.* *_init */
bool dwfltab_init(void)
{
  dwfltab *htab = &default_table;
  htab->size = DWFLTAB_DEFAULT_SIZE;
  htab->filled = 0;
  htab->table = calloc ((htab->size + 1), sizeof(htab->table[0]));
  return (htab->table != NULL);
}

/* XXX based on lib/dynamicsizehash.* insert_entry_2 */
bool dwfltab_resize(void)
{
  /* TODO: Also implement LRU eviction, especially given the number of
     extremely-short-lived processes seen on GNOME desktop. */
  dwfltab *htab = &default_table;
  ssize_t old_size = htab->size;
  dwfltab_ent *old_table = htab->table;
  htab->size = next_prime (htab->size * 2);
  htab->table = calloc ((htab->size + 1), sizeof(htab->table[0]));
  if (htab->table == NULL)
    {
      htab->size = old_size;
      htab->table = old_table;
      return false;
    }
  htab->filled = 0;
  /* Transfer the old entries to the new table. */
  for (ssize_t idx = 1; idx <= old_size; ++idx)
    if (old_table[idx].used)
      {
	dwfltab_ent *ent0 = &old_table[idx];
	dwfltab_ent *ent1 = dwfltab_find(ent0->pid);
	assert (ent1 != NULL);
	memcpy (ent1, ent0, sizeof(dwfltab_ent));
      }
  free (old_table);
  /* TODO: Need to decide log level for this message. For now, it's
     not a failure, and printing it by default seems harmless: */
  fprintf(stderr, N_("Resized Dwfl table from %ld to %ld, copied %ld entries\n"),
	  old_size, htab->size, htab->filled);
  return true;
}

/* XXX based on lib/dynamicsizehash.* *_find */
dwfltab_ent *dwfltab_find(pid_t pid)
{
  dwfltab *htab = &default_table;
  ssize_t idx = 1 + (htab->size > (ssize_t)pid ? (ssize_t)pid : (ssize_t)pid % htab->size);

  if (!htab->table[idx].used)
    goto found;
  if (htab->table[idx].pid == pid)
    goto found;

  int64_t hash = 1 + pid % (htab->size - 2);
  do
    {
      if (idx <= hash)
	idx = htab->size + idx - hash;
      else
	idx -= hash;

      if (!htab->table[idx].used)
	goto found;
      if (htab->table[idx].pid == pid)
	goto found;
    }
  while (true);

 found:
  if (htab->table[idx].pid == 0)
    {
      if (100 * htab->filled > 90 * htab->size)
	if (!dwfltab_resize())
	  return NULL;
      htab->table[idx].used = true;
      htab->table[idx].pid = pid;
      htab->filled += 1;
    }
  return &htab->table[idx];
}

Dwfl *
pid_find_dwfl (pid_t pid)
{
  dwfltab_ent *entry = dwfltab_find(pid);
  if (entry == NULL)
    return NULL;
  return entry->dwfl;
}

char *
pid_find_comm (pid_t pid)
{
  dwfltab_ent *entry = dwfltab_find(pid);
  if (entry == NULL)
    return "<unknown>";
  if (entry->comm != NULL)
    return entry->comm;
  char name[64];
  int i = snprintf (name, sizeof(name), "/proc/%ld/comm", (long) pid);
  FILE *procfile = fopen(name, "r");
  if (procfile == NULL)
    goto fail;
  size_t linelen = 0;
  i = getline(&entry->comm, &linelen, procfile);
  if (i < 0)
    {
      free(entry->comm);
      goto fail;
    }
  for (i = linelen - 1; i > 0; i--)
    if (entry->comm[i] == '\n')
	entry->comm[i] = '\0';
  fclose(procfile);
  goto done;
 fail:
  entry->comm = (char *)malloc(16);
  snprintf (entry->comm, 16, "<unknown>");
 done:
  return entry->comm;
}

/* Cache dwfl structs in a basic hash table. */
void
pid_store_dwfl (pid_t pid, Dwfl *dwfl)
{
  dwfltab_ent *entry = dwfltab_find(pid);
  if (entry == NULL)
    return;
  entry->pid = pid;
  entry->dwfl = dwfl;
  pid_find_comm(pid);
  return;
}

/*****************************************************
 * Sysprof backend: basic none/passthrough callbacks *
 *****************************************************/

#if HAVE_SYSPROF_HEADERS

int
sysprof_none_cb (SysprofCaptureFrame *frame __attribute__ ((unused)),
		 void *arg __attribute__ ((unused)))
{
  return SYSPROF_CB_OK;
}

struct sysprof_passthru_info
{
  int output_fd;
  SysprofReader *reader;
  int pos; /* for diagnostic purposes */
};

int
sysprof_passthru_cb (SysprofCaptureFrame *frame, void *arg)
{
  struct sysprof_passthru_info *spi = (struct sysprof_passthru_info *)arg;
  sysprof_reader_bswap_frame (spi->reader, frame); /* reverse the earlier bswap */
  ssize_t n_write = write (spi->output_fd, frame, frame->len);
  spi->pos += frame->len;
  assert ((spi->pos % SYSPROF_CAPTURE_ALIGN) == 0);
  if (n_write < 0)
    error (EXIT_BAD, errno, N_("Write error to file or FIFO '%s'"), output_path);
  return SYSPROF_CB_OK;
}

#endif /* HAVE_SYSPROF_HEADERS */

/****************************************
 * Sysprof backend: unwinding callbacks *
 ****************************************/

#if HAVE_SYSPROF_HEADERS

#define UNWIND_ADDR_INCREMENT 512
struct sysprof_unwind_info
{
  int output_fd;
  SysprofReader *reader;
  int pos; /* for diagnostic purposes */
  int n_addrs;
  int max_addrs; /* for diagnostic purposes */
  uint64_t last_abi;
  Dwarf_Addr last_base; /* for diagnostic purposes */
  Dwarf_Addr last_sp; /* for diagnostic purposes */
#ifdef DEBUG_MODULES
  Dwfl *last_dwfl; /* for diagnostic purposes */
#endif
  pid_t last_pid; /* for diagnostic purposes, to provide access to dwfltab */
  Dwarf_Addr *addrs; /* allocate blocks of UNWIND_ADDR_INCREMENT */
  void *outbuf;
};

#ifdef FIND_DEBUGINFO

static char *debuginfo_path = NULL;

static const Dwfl_Callbacks sample_callbacks =
  {
    .find_elf = dwfl_linux_proc_find_elf,
    .find_debuginfo = dwfl_standard_find_debuginfo,
    .debuginfo_path = &debuginfo_path,
  };

#else

int
nop_find_debuginfo (Dwfl_Module *mod __attribute__((unused)),
		    void **userdata __attribute__((unused)),
		    const char *modname __attribute__((unused)),
		    GElf_Addr base __attribute__((unused)),
		    const char *file_name __attribute__((unused)),
		    const char *debuglink_file __attribute__((unused)),
		    GElf_Word debuglink_crc __attribute__((unused)),
		    char **debuginfo_file_name __attribute__((unused)))
{
#ifdef DEBUG_MODULES
  fprintf(stderr, "nop_find_debuginfo: modname=%s file_name=%s debuglink_file=%s\n",
	  modname, file_name, debuglink_file);
#endif
  return -1;
}

static const Dwfl_Callbacks sample_callbacks =
{
  .find_elf = dwfl_linux_proc_find_elf,
  .find_debuginfo = nop_find_debuginfo, /* work with CFI only */
};

#endif /* FIND_DEBUGINFO */

/* TODO: Probably needs to be relocated to libdwfl/linux-pid-attach.c
   to remove a dependency on the private dwfl interface. */
int
find_procfile (Dwfl *dwfl, pid_t *pid, Elf **elf, int *elf_fd)
{
  char buffer[36];
  FILE *procfile;
  int err = 0; /* The errno to return and set for dwfl->attacherr.  */

  /* Make sure to report the actual PID (thread group leader) to
     dwfl_attach_state.  */
  snprintf (buffer, sizeof (buffer), "/proc/%ld/status", (long) *pid);
  procfile = fopen (buffer, "r");
  if (procfile == NULL)
    {
      err = errno;
    fail:
      if (dwfl->process == NULL && dwfl->attacherr == DWFL_E_NOERROR) /* XXX requires libdwflP.h */
	{
	  errno = err;
	  /* TODO: __libdwfl_canon_error not exported from libdwfl */
	  /* dwfl->attacherr = __libdwfl_canon_error (DWFL_E_ERRNO); */
	}
      return err;
    }

  char *line = NULL;
  size_t linelen = 0;
  while (getline (&line, &linelen, procfile) >= 0)
    if (startswith (line, "Tgid:"))
      {
	errno = 0;
	char *endptr;
	long val = strtol (&line[5], &endptr, 10);
	if ((errno == ERANGE && val == LONG_MAX)
	    || *endptr != '\n' || val < 0 || val != (pid_t) val)
	  *pid = 0;
	else
	  *pid = (pid_t) val;
	break;
      }
  free (line);
  fclose (procfile);

  if (*pid == 0)
    {
      err = ESRCH;
      goto fail;
    }

  char name[64];
  int i = snprintf (name, sizeof (name), "/proc/%ld/task", (long) *pid);
  if (i <= 0 || i >= (ssize_t) sizeof (name) - 1)
    {
      errno = -ENOMEM;
      goto fail;
    }
  DIR *dir = opendir (name);
  if (dir == NULL)
    {
      err = errno;
      goto fail;
    }
  else
    closedir(dir);

  i = snprintf (name, sizeof (name), "/proc/%ld/exe", (long) *pid);
  assert (i > 0 && i < (ssize_t) sizeof (name) - 1);
  *elf_fd = open (name, O_RDONLY);
  if (*elf_fd >= 0)
    {
      *elf = elf_begin (*elf_fd, ELF_C_READ_MMAP, NULL);
      if (*elf == NULL)
	{
	  /* Just ignore, dwfl_attach_state will fall back to trying
	     to associate the Dwfl with one of the existing Dwfl_Module
	     ELF images (to know the machine/class backend to use).  */
	  if (show_failures)
	    fprintf(stderr, N_("find_procfile pid %lld: elf not found"),
		    (long long)*pid);
	  close (*elf_fd);
	  *elf_fd = -1;
	}
    }
  else
    *elf = NULL;
  return 0;
}

Dwfl *
sysprof_init_dwfl (struct sysprof_unwind_info *sui,
		   SysprofCaptureStackUser *ev,
		   SysprofCaptureUserRegs *regs)
{
  pid_t pid = ev->frame.pid;
  /* TODO: Need to generalize this code beyond x86 architectures. */
  /* XXX: Note that sysprof requesting the x86_64 register file from
     perf_events will result in an array of 17 regs even for 32-bit
     applications. */
#define EXPECTED_REGS 17
  if (regs->n_regs < EXPECTED_REGS) /* XXX expecting everything except FLAGS */
    {
      if (show_failures)
	fprintf(stderr, N_("sysprof_init_dwfl: n_regs=%d, expected %d\n"),
		regs->n_regs, EXPECTED_REGS);
      return NULL;
    }

  Dwfl *dwfl = pid_find_dwfl(pid);
  struct __sample_arg *sample_arg;
  bool cached = false;
  if (dwfl != NULL)
    {
      sample_arg = dwfl->process->callbacks_arg; /* XXX requires libdwflP.h */
      cached = true;
      goto reuse;
    }
  dwfl = dwfl_begin (&sample_callbacks);

  int err = dwfl_linux_proc_report (dwfl, pid);
  if (err < 0)
    {
      if (show_failures)
	fprintf(stderr, "dwfl_linux_proc_report pid %lld: %s",
		(long long) pid, dwfl_errmsg (-1));
      return NULL;
    }
  err = dwfl_report_end (dwfl, NULL, NULL);
  if (err != 0)
    {
      if (show_failures)
	fprintf(stderr, "dwfl_report_end pid %lld: %s",
		(long long) pid, dwfl_errmsg (-1));
      return NULL;
    }

  Elf *elf = NULL;
  int elf_fd = -1;
  err = find_procfile (dwfl, &pid, &elf, &elf_fd);
  if (err < 0)
    {
      if (show_failures)
	fprintf(stderr, "find_procfile pid %lld: %s",
		(long long) pid, dwfl_errmsg (-1));
      return NULL;
    }

  sample_arg = malloc (sizeof *sample_arg);
  if (sample_arg == NULL)
    {
      if (elf != NULL) {
	elf_end (elf);
	close (elf_fd);
      }
      free (sample_arg);
      return NULL;
    }

 reuse:
  sample_arg->tid = ev->tid;
  sample_arg->size = ev->size;
  sample_arg->data = (uint8_t *)&ev->data;
  sample_arg->regs = regs->regs;
  sample_arg->n_regs = (uint64_t)regs->n_regs;
  sample_arg->abi = (uint64_t)regs->abi;
  /* TODO: Need to generalize this code beyond x86 architectures. */
  /* Register ordering cf. linux arch/x86/include/uapi/asm/perf_regs.h enum perf_event_x86_regs: */
  sample_arg->sp = regs->regs[7];
  sample_arg->pc = regs->regs[8];
  sample_arg->base_addr = sample_arg->sp;
  sui->last_sp = sample_arg->base_addr;
  sui->last_base = sample_arg->base_addr;

  if (show_frames) {
    bool is_abi32 = (sample_arg->abi == PERF_SAMPLE_REGS_ABI_32);
    fprintf(stderr, "sysprof_init_dwfl pid %lld%s: size=%ld%s pc=%lx sp=%lx+(%lx)\n",
	    (long long) pid, cached ? " (cached)" : "",
	    sample_arg->size, is_abi32 ? " (32-bit)" : "",
	    sample_arg->pc, sample_arg->base_addr,
	    sample_arg->sp - sample_arg->base_addr);
  }

  if (!cached && ! dwfl_attach_state (dwfl, elf, pid, &sample_thread_callbacks, sample_arg))
    {
      if (show_failures)
	fprintf(stderr, "dwfl_attach_state pid %lld: %s\n",
		(long long)pid, dwfl_errmsg(-1));
      elf_end (elf);
      close (elf_fd);
      free (sample_arg);
      return NULL;
    }
  if (!cached)
    pid_store_dwfl (pid, dwfl);
  return dwfl;
}

int
sysprof_unwind_frame_cb (Dwfl_Frame *state, void *arg)
{
  Dwarf_Addr pc;
  bool isactivation;
  if (! dwfl_frame_pc (state, &pc, &isactivation))
    {
      if (show_failures)
	fprintf(stderr, "dwfl_frame_pc: %s\n",
		dwfl_errmsg(-1));
      return DWARF_CB_ABORT;
    }

  Dwarf_Addr pc_adjusted = pc - (isactivation ? 0 : 1);
  Dwarf_Addr sp;
  /* TODO: Need to generalize this code beyond x86 architectures. */
  struct sysprof_unwind_info *sui = (struct sysprof_unwind_info *)arg;
  int is_abi32 = (sui->last_abi == PERF_SAMPLE_REGS_ABI_32);
  /* DWARF register order cf. elfutils backends/{x86_64,i386}_initreg.c: */
  int user_regs_sp = is_abi32 ? 4 : 7;
  int rc = dwfl_frame_reg (state, user_regs_sp, &sp);
  if (rc < 0)
    {
      if (show_failures)
	fprintf(stderr, "dwfl_frame_reg: %s\n",
		dwfl_errmsg(-1));
      return DWARF_CB_ABORT;
    }

#ifdef DEBUG_MODULES
  Dwfl_Module *mod = dwfl_addrmodule(sui->last_dwfl, pc);
  if (mod == NULL)
    {
      fprintf(stderr, "* pc=%lx -> NO MODULE\n", pc);
    }
  else
    {
      const char *mainfile;
      const char *debugfile;
      const char *modname = dwfl_module_info (mod, NULL, NULL, NULL, NULL,
					      NULL, &mainfile, &debugfile);
      fprintf (stderr, "* module %s -> mainfile=%s debugfile=%s\n", modname, mainfile, debugfile);
      Dwarf_Addr bias;
      Dwarf_CFI *cfi_eh = dwfl_module_eh_cfi (mod, &bias);
      if (cfi_eh == NULL)
	fprintf(stderr, "* pc=%lx -> NO EH_CFI\n", pc);
    }
#endif

  dwfltab_ent *dwfl_ent = dwfltab_find(sui->last_pid);
  if (dwfl_ent != NULL)
    {
      Dwfl_Unwound_Source unwound_source = dwfl_frame_unwound_source(state);
      if (unwound_source > dwfl_ent->worst_unwound)
	dwfl_ent->worst_unwound = unwound_source;
      dwfl_ent->last_unwound = unwound_source;
      if (show_frames)
	fprintf(stderr, "* frame %d: pc_adjusted=%lx sp=%lx+(%lx) [%s]\n",
		sui->n_addrs, pc_adjusted, sui->last_base, sp - sui->last_base,
		dwfl_unwound_source_str(unwound_source));
    }
  else
    {
      if (show_frames)
	fprintf(stderr, N_("* frame %d: pc_adjusted=%lx sp=%lx+(%lx) [dwfl_ent not found]\n"),
		sui->n_addrs, pc_adjusted, sui->last_base, sp - sui->last_base);
    }

  if (sui->n_addrs > maxframes)
    {
      /* XXX very rarely, the unwinder can loop infinitely; worth investigating? */
      if (show_failures)
	fprintf(stderr, N_("sysprof_unwind_frame_cb: sample exceeded maxframes %d\n"),
		maxframes);
      return DWARF_CB_ABORT;
    }

  sui->last_sp = sp;
  if (sui->n_addrs >= sui->max_addrs)
    {
      sui->addrs = reallocarray (sui->addrs, sui->max_addrs + UNWIND_ADDR_INCREMENT, sizeof(Dwarf_Addr));
      sui->max_addrs = sui->max_addrs + UNWIND_ADDR_INCREMENT;
    }
  sui->addrs[sui->n_addrs] = pc;
  sui->n_addrs++;
  return DWARF_CB_OK;
}

int
sysprof_unwind_cb (SysprofCaptureFrame *frame, void *arg)
{
  struct sysprof_unwind_info *sui = (struct sysprof_unwind_info *)arg;
  ssize_t n_write;

  /* additional diagnostic to display process name */
  char *comm = NULL;
  if (frame->type == SYSPROF_CAPTURE_FRAME_SAMPLE || frame->type == SYSPROF_CAPTURE_FRAME_STACK_USER)
      comm = pid_find_comm(frame->pid);

  if (frame->type == SYSPROF_CAPTURE_FRAME_SAMPLE)
    {
      /* XXX additional diagnostics for comparing to eu-stacktrace unwind */
      SysprofCaptureSample *ev_sample = (SysprofCaptureSample *)frame;
      if (show_samples)
	fprintf(stderr, N_("sysprof_unwind_cb pid %lld (%s): callchain sample with %d frames\n"),
		(long long)frame->pid, comm, ev_sample->n_addrs);
      if (show_summary)
	{
	  /* For final diagnostics. */
	  dwfltab_ent *dwfl_ent = dwfltab_find(frame->pid);
	  if (dwfl_ent == NULL && show_failures)
	    fprintf(stderr, N_("sysprof_unwind_cb pid %lld (%s): could not create Dwfl table entry\n"),
		    (long long)frame->pid, comm);
	  else if (dwfl_ent != NULL)
	    {
	      if (ev_sample->n_addrs > dwfl_ent->max_frames)
		dwfl_ent->max_frames = ev_sample->n_addrs;
	      dwfl_ent->total_samples ++;
	      if (ev_sample->n_addrs <= 2)
		dwfl_ent->lost_samples ++;
	    }
	}
    }
  if (frame->type != SYSPROF_CAPTURE_FRAME_STACK_USER)
    {
      sysprof_reader_bswap_frame (sui->reader, frame); /* reverse the earlier bswap */
      n_write = write (sui->output_fd, frame, frame->len);
      sui->pos += frame->len;
      assert ((sui->pos % SYSPROF_CAPTURE_ALIGN) == 0);
      if (n_write < 0)
	error (EXIT_BAD, errno, N_("Write error to file or FIFO '%s'"), output_path);
      return SYSPROF_CB_OK;
    }
  SysprofCaptureStackUser *ev = (SysprofCaptureStackUser *)frame;
  uint8_t *tail_ptr = (uint8_t *)ev;
  tail_ptr += sizeof(SysprofCaptureStackUser) + ev->size;
  SysprofCaptureUserRegs *regs = (SysprofCaptureUserRegs *)tail_ptr;
  if (show_frames)
    fprintf(stderr, "\n"); /* extra newline for padding */
  Dwfl *dwfl = sysprof_init_dwfl (sui, ev, regs);
  if (dwfl == NULL)
    {
      if (show_summary)
	{
	  dwfltab_ent *dwfl_ent = dwfltab_find(frame->pid);
	  dwfl_ent->total_samples++;
	  dwfl_ent->lost_samples++;
	}
      if (show_failures)
	fprintf(stderr, "sysprof_init_dwfl pid %lld (%s) (failed)\n",
		(long long)frame->pid, comm);
      return SYSPROF_CB_OK;
    }
  sui->n_addrs = 0;
  sui->last_abi = regs->abi;
#ifdef DEBUG_MODULES
  sui->last_dwfl = dwfl;
#endif
  sui->last_pid = frame->pid;
  int rc = dwfl_getthread_frames (dwfl, ev->tid, sysprof_unwind_frame_cb, sui);
  if (rc < 0)
    {
      if (show_failures)
	fprintf(stderr, "dwfl_getthread_frames pid %lld: %s\n",
		(long long)frame->pid, dwfl_errmsg(-1));
    }
  if (show_samples)
    {
      bool is_abi32 = (regs->abi == PERF_SAMPLE_REGS_ABI_32);
      fprintf(stderr, N_("sysprof_unwind_cb pid %lld (%s)%s: unwound %d frames\n"),
	      (long long)frame->pid, comm, is_abi32 ? " (32-bit)" : "", sui->n_addrs);
    }
  if (show_summary)
    {
      /* For final diagnostics. */
      dwfltab_ent *dwfl_ent = dwfltab_find(frame->pid);
      if (dwfl_ent != NULL && sui->n_addrs > dwfl_ent->max_frames)
	dwfl_ent->max_frames = sui->n_addrs;
      dwfl_ent->total_samples++;
      if (sui->n_addrs <= 2)
	dwfl_ent->lost_samples ++;
    }

  /* Assemble and output callchain frame. */
  /* XXX assert(sizeof(Dwarf_Addr) == sizeof(SysprofCaptureAddress)); */
  SysprofCaptureSample *ev_callchain;
  size_t len = sizeof *ev_callchain + (sui->n_addrs * sizeof(Dwarf_Addr));
  ev_callchain = (SysprofCaptureSample *)sui->outbuf;
  if (len > USHRT_MAX)
    {
      if (show_failures)
	fprintf(stderr, N_("sysprof_unwind_cb frame size %ld is too large (max %d)\n"),
		len, USHRT_MAX);
      return SYSPROF_CB_OK;
    }
  SysprofCaptureFrame *out_frame = (SysprofCaptureFrame *)ev_callchain;
  out_frame->len = len;
  out_frame->cpu = ev->frame.cpu;
  out_frame->pid = ev->frame.pid;
  out_frame->time = ev->frame.time;
  out_frame->type = SYSPROF_CAPTURE_FRAME_SAMPLE;
  out_frame->padding1 = 0;
  out_frame->padding2 = 0;
  ev_callchain->n_addrs = sui->n_addrs;
  ev_callchain->tid = ev->tid;
  memcpy (ev_callchain->addrs, sui->addrs, (sui->n_addrs * sizeof(SysprofCaptureAddress)));
  n_write = write (sui->output_fd, ev_callchain, len);
  if (n_write < 0)
    error (EXIT_BAD, errno, N_("Write error to file or FIFO '%s'"), output_path);
  return SYSPROF_CB_OK;
}

#endif /* HAVE_SYSPROF_HEADERS */

/****************
 * Main program *
 ****************/

/* Required to match our signal handling with that of a sysprof parent process. */
static void sigint_handler (int /* signo */)
{
  if (signal_count >= 2)
    {
      exit(1);
    }

  if (signal_count == 0)
    {
      fprintf (stderr, "%s\n", N_("Waiting for input to finish. Press twice more ^C to force exit."));
    }

  signal_count ++;
}

static error_t
parse_opt (int key, char *arg __attribute__ ((unused)),
	   struct argp_state *state)
{
  switch (key)
    {
    case 'i':
      input_path = arg;
      break;

    case 'o':
      output_path = arg;
      break;

    case 'm':
      if (strcmp (arg, "none") == 0)
	{
	  processing_mode = MODE_NONE;
	}
      else if (strcmp (arg, "passthru") == 0)
	{
	  processing_mode = MODE_PASSTHRU;
	}
      else if (strcmp (arg, "naive") == 0)
	{
	  processing_mode = MODE_NAIVE;
	}
      else
	{
	  argp_error (state, N_("Unsupported -m '%s', should be " MODE_OPTS "."), arg);
	}
      break;

    case 'f':
      if (strcmp (arg, "sysprof") == 0)
	{
	  input_format = FORMAT_SYSPROF;
	}
      else
	{
	  argp_error (state, N_("Unsupported -f '%s', should be " FORMAT_OPTS "."), arg);
	}
      break;

    case OPT_DEBUG:
#ifdef DEBUG_MEMORY_READ
      show_memory_reads = true;
#endif
      show_frames = true;
      FALLTHROUGH;
    case 'v':
      show_samples = true;
      show_failures = true;
      show_summary = true;
      break;

    case ARGP_KEY_END:
      if (input_path == NULL)
	input_path = "-"; /* default to stdin */

      if (output_path == NULL)
	output_path = "-"; /* default to stdout */

      if (processing_mode == 0)
	processing_mode = MODE_NAIVE;

      if (input_format == 0)
	input_format = FORMAT_SYSPROF;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  /* Set locale. */
  (void) setlocale (LC_ALL, "");

  const struct argp_option options[] =
    {
      { NULL, 0, NULL, 0, N_("Input and output selection options:"), 0 },
      { "input", 'i', "PATH", 0,
	N_("File or FIFO to read stack samples from"), 0 },
      /* TODO: Should also support taking an FD for fork/exec pipes. */
      { "output", 'o', "PATH", 0,
	N_("File or FIFO to send stack traces to"), 0 },

      { NULL, 0, NULL, 0, N_("Processing options:"), 0 },
      { "mode", 'm', MODE_OPTS, 0,
	N_("Processing mode, default 'naive'"), 0 },
      /* TODO: Should also support 'naive', 'caching'. */
      /* TODO: Add an option to control stack-stitching. */
      { "verbose", 'v', NULL, 0,
	N_("Show additional information for each unwound sample"), 0 },
      { "debug", OPT_DEBUG, NULL, 0,
	N_("Show additional information for each unwound frame"), 0 },
      /* TODO: Add a 'quiet' option suppressing summaries + errors.
         Perhaps also allow -v, -vv, -vvv in SystemTap style? */
      { "format", 'f', FORMAT_OPTS, 0,
	N_("Input data format, default 'sysprof'"), 0 },
      /* TODO: Add an option to control output data format separately,
	 shift to I/O selection section. */
      { NULL, 0, NULL, 0, NULL, 0 }
    };

  const struct argp argp =
    {
      .options = options,
      .parser = parse_opt,
      .doc = N_("Process a stream of stack samples into stack traces.\n\
\n\
Experimental tool, see README.eu-stacktrace in the development branch:\n\
https://sourceware.org/cgit/elfutils/tree/README.eu-stacktrace?h=users/serhei/eu-stacktrace\n")
    };

  argp_parse(&argp, argc, argv, 0, NULL, NULL);

  /* Also handle ELFUTILS_STACKTRACE_VERBOSE_ENV_VAR: */
  char *env_verbose = getenv(ELFUTILS_STACKTRACE_VERBOSE_ENV_VAR);
  if (env_verbose == NULL || strlen(env_verbose) == 0)
    ; /* nop, use command line options */
  else if (strcmp(env_verbose, "false") == 0
	   || strcmp(env_verbose, "0") == 0)
    ; /* nop, use command line options */
  else if (strcmp(env_verbose, "true") == 0
	   || strcmp(env_verbose, "verbose") == 0
	   || strcmp(env_verbose, "1") == 0)
    {
      show_samples = true;
      show_failures = true;
      show_summary = true;
    }
  else if (strcmp(env_verbose, "debug") == 0
	   || strcmp(env_verbose, "2") == 0)
    {
#ifdef DEBUG_MEMORY_READ
      show_memory_reads = true;
#endif
      show_frames = true;
      show_samples = true;
      show_failures = true;
      show_summary = true;
    }
  else
    fprintf (stderr, N_("WARNING: Unknown value '%s' in environment variable %s, ignoring\n"),
	     env_verbose, ELFUTILS_STACKTRACE_VERBOSE_ENV_VAR);

  /* TODO: Also handle common expansions e.g. ~/foo instead of /home/user/foo. */
  if (strcmp (input_path, "-") == 0)
    input_fd = STDIN_FILENO;
  else
    input_fd = open (input_path, O_RDONLY);
  if (input_fd < 0)
    error (EXIT_BAD, errno, N_("Cannot open input file or FIFO '%s'"), input_path);
  if (strcmp (output_path, "-") == 0)
    output_fd = STDOUT_FILENO;
  else
    output_fd = open (output_path, O_CREAT | O_WRONLY, 0640);
  if (output_fd < 0)
    error (EXIT_BAD, errno, N_("Cannot open output file or FIFO '%s'"), output_path);

  /* TODO: Only really needed if launched from sysprof and inheriting its signals. */
  if (signal (SIGINT, sigint_handler) == SIG_ERR)
    error (EXIT_BAD, errno, N_("Cannot set signal handler for SIGINT"));

#if !(HAVE_SYSPROF_HEADERS)
  /* TODO: Should hide corresponding command line options when this is the case? */
  error (EXIT_BAD, 0, N_("Sysprof support is not available in this version."));

  /* XXX: The following are not specific to the Sysprof backend;
     avoid unused-variable warnings while it is the only backend. */
  (void)sample_thread_callbacks;
  (void)output_format;
  (void)maxframes;
#else
  fprintf(stderr, "\n=== starting eu-stacktrace ===\n");

  /* TODO: For now, code the processing loop for sysprof only; generalize later. */
  assert (input_format == FORMAT_SYSPROF);
  assert (output_format == FORMAT_SYSPROF);
  SysprofReader *reader = sysprof_reader_begin (input_fd);
  ssize_t n_write = write (output_fd, &reader->header, sizeof reader->header);
  if (n_write < 0)
    error (EXIT_BAD, errno, N_("Write error to file or FIFO '%s'"), output_path);
  ptrdiff_t offset;
  unsigned long int output_pos = 0;
  if (processing_mode == MODE_NONE)
    {
      struct sysprof_passthru_info sni = { output_fd, reader, sizeof reader->header };
      offset = sysprof_reader_getframes (reader, &sysprof_none_cb, &sni);
      output_pos = sni.pos;
    }
  else if (processing_mode == MODE_PASSTHRU)
    {
      struct sysprof_passthru_info spi = { output_fd, reader, sizeof reader->header };
      offset = sysprof_reader_getframes (reader, &sysprof_passthru_cb, &spi);
      output_pos = spi.pos;
    }
  else /* processing_mode == MODE_NAIVE */
    {
      if (!dwfltab_init())
	error (EXIT_BAD, errno, N_("Could not initialize Dwfl table"));
      struct sysprof_unwind_info sui;
      sui.output_fd = output_fd;
      sui.reader = reader;
      sui.pos = sizeof reader->header;
      sui.n_addrs = 0;
      sui.last_base = 0;
      sui.last_sp = 0;
      sui.last_abi = PERF_SAMPLE_REGS_ABI_NONE;
      sui.max_addrs = UNWIND_ADDR_INCREMENT;
      sui.addrs = (Dwarf_Addr *)malloc (sui.max_addrs * sizeof(Dwarf_Addr));
      sui.outbuf = (void *)malloc (USHRT_MAX * sizeof(uint8_t));
      offset = sysprof_reader_getframes (reader, &sysprof_unwind_cb, &sui);
      if (show_summary)
	{
	  /* Final diagnostics. */
#define PERCENT(x,tot) ((x+tot == 0)?0.0:((double)x)/((double)tot)*100.0)
	  int total_samples = 0;
	  int total_lost_samples = 0;
	  fprintf(stderr, "\n=== final summary ===\n");
	  for (unsigned idx = 1; idx < default_table.size; idx++)
	    {
	      dwfltab_ent *t = default_table.table;
	      if (!t[idx].used)
		continue;
	      /* XXX worst_unwound gives least preferred unwind method used for this process
		 (i.e. eh_frame is preferred to dwarf is preferred to ebl) */
	      fprintf(stderr, N_("%d %s -- max %d frames, received %d samples, lost %d samples (%.1f%%) (last %s, worst %s)\n"),
		      t[idx].pid, t[idx].comm, t[idx].max_frames,
		      t[idx].total_samples, t[idx].lost_samples,
		      PERCENT(t[idx].lost_samples, t[idx].total_samples),
		      dwfl_unwound_source_str(t[idx].last_unwound),
		      dwfl_unwound_source_str(t[idx].worst_unwound));
	      total_samples += t[idx].total_samples;
	      total_lost_samples += t[idx].lost_samples;
	    }
	  fprintf(stderr, "===\n");
	  fprintf(stderr, N_("TOTAL -- received %d samples, lost %d samples, loaded %ld processes\n"),
		  total_samples, total_lost_samples,
		  default_table.filled /* TODO: after implementing LRU eviction, need to maintain a separate count, e.g. htab->filled + htab->evicted */);
	}
      output_pos = sui.pos;
      free(sui.addrs);
      free(sui.outbuf);
    }
  if (offset < 0 && output_pos <= sizeof reader->header)
    error (EXIT_BAD, errno, N_("No frames in file or FIFO '%s'"), input_path);
  else if (offset < 0)
    error (EXIT_BAD, errno, N_("Error processing file or FIFO '%s' at input offset %ld, output offset %ld"),
	   input_path, reader->pos, output_pos);
  sysprof_reader_end (reader);
#endif

  if (input_fd != -1)
    close (input_fd);
  if (output_fd != -1)
    close (output_fd);

  return EXIT_OK;
}
