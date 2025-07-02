/* Get Dwarf Frame state for perf stack sample data.
   Copyright (C) 2025 Red Hat, Inc.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if defined(__linux__)
# include <linux/perf_event.h>
#endif

#include "libdwfl_stacktraceP.h"

Ebl *default_ebl = NULL;
GElf_Half default_ebl_machine = EM_NONE;

uint64_t dwflst_perf_sample_preferred_regs_mask (GElf_Half machine)
{
  /* XXX The most likely case is that this will only be called once,
     for the current architecture.  So we keep one Ebl* around for
     answering this query and replace it in the unlikely case of
     getting called with different architectures.  */
  if (default_ebl != NULL && default_ebl_machine != machine)
    {
      ebl_closebackend(default_ebl);
      default_ebl = NULL;
    }
  if (default_ebl == NULL)
    {
      default_ebl = ebl_openbackend_machine(machine);
      default_ebl_machine = machine;
    }
  if (default_ebl != NULL)
    return ebl_perf_frame_regs_mask (default_ebl);
  return 0;
}

struct perf_sample_info {
  pid_t pid;
  pid_t tid;
  Dwarf_Addr base_addr;
  const uint8_t *stack;
  size_t stack_size;
  const Dwarf_Word *regs;
  uint n_regs;
  uint64_t perf_regs_mask;
  uint abi;
  Dwarf_Addr pc;
};

/* The next few functions imitate the corefile interface for a single
   stack sample, with very restricted access to registers and memory. */

/* Just yield the single thread id matching the sample. */
static pid_t
sample_next_thread (Dwfl *dwfl __attribute__ ((unused)), void *dwfl_arg,
		    void **thread_argp)
{
  struct perf_sample_info *sample_arg =
    (struct perf_sample_info *)dwfl_arg;
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
  struct perf_sample_info *sample_arg =
    (struct perf_sample_info *)dwfl_arg;
  *thread_argp = (void *)sample_arg;
  if (sample_arg->tid != tid)
    {
      __libdwfl_seterrno(DWFL_E_INVALID_ARGUMENT);
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
  struct perf_sample_info *sample_arg =
    (struct perf_sample_info *)arg;
  Dwfl_Module *mod = INTUSE(dwfl_addrmodule) (dwfl, addr);
  Dwarf_Addr bias;
  Elf_Scn *section = INTUSE(dwfl_module_address_section) (mod, &addr, &bias);

  if (!section)
    {
      __libdwfl_seterrno(DWFL_E_ADDR_OUTOFRANGE);
      return false;
    }

  Elf_Data *data = elf_getdata(section, NULL);
  if (data && data->d_buf && data->d_size > addr) {
    uint8_t *d = ((uint8_t *)data->d_buf) + addr;
    copy_word(result, d, sample_arg->abi);
    return true;
  }
  __libdwfl_seterrno(DWFL_E_ADDR_OUTOFRANGE);
  return false;
}

static bool
sample_memory_read (Dwfl *dwfl, Dwarf_Addr addr, Dwarf_Word *result, void *arg)
{
  struct perf_sample_info *sample_arg =
    (struct perf_sample_info *)arg;
  /* Imitate read_cached_memory() with the stack sample data as the cache. */
  if (addr < sample_arg->base_addr ||
      addr - sample_arg->base_addr >= sample_arg->stack_size)
    return elf_memory_read(dwfl, addr, result, arg);
  const uint8_t *d = &sample_arg->stack[addr - sample_arg->base_addr];
  copy_word(result, d, sample_arg->abi);
  return true;
}

static bool
sample_set_initial_registers (Dwfl_Thread *thread, void *arg)
{
  struct perf_sample_info *sample_arg =
    (struct perf_sample_info *)arg;
  INTUSE(dwfl_thread_state_register_pc) (thread, sample_arg->pc);
  Dwfl_Process *process = thread->process;
  Ebl *ebl = process->ebl;
  return ebl_set_initial_registers_sample
    (ebl, sample_arg->regs, sample_arg->n_regs,
     sample_arg->perf_regs_mask, sample_arg->abi,
     __libdwfl_set_initial_registers_thread, thread);
}

static void
sample_detach (Dwfl *dwfl __attribute__ ((unused)), void *dwfl_arg)
{
  struct perf_sample_info *sample_arg =
    (struct perf_sample_info *)dwfl_arg;
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

int
dwflst_perf_sample_getframes (Dwfl *dwfl, Elf *elf,
			      pid_t pid, pid_t tid,
			      const void *stack, size_t stack_size,
			      const Dwarf_Word *regs, uint n_regs,
			      uint64_t perf_regs_mask, uint abi,
			      int (*callback) (Dwfl_Frame *state, void *arg),
			      void *arg)
{
  /* TODO: Lock the dwfl to ensure attach_state does not interfere
     with other dwfl_perf_sample_getframes calls. */

  struct perf_sample_info *sample_arg;
  bool attached = false;
  if (dwfl->process != NULL)
    {
      sample_arg = dwfl->process->callbacks_arg;
      attached = true;
    }
  else
    {
      sample_arg = malloc (sizeof *sample_arg);
      if (sample_arg == NULL)
	{
	  __libdwfl_seterrno(DWFL_E_NOMEM);
	  return -1;
	}
    }

  sample_arg->pid = pid;
  sample_arg->tid = tid;
  sample_arg->stack = (const uint8_t *)stack;
  sample_arg->stack_size = stack_size;
  sample_arg->regs = regs;
  sample_arg->n_regs = n_regs;
  sample_arg->perf_regs_mask = perf_regs_mask;
  sample_arg->abi = abi;

  if (! attached
      && ! INTUSE(dwfl_attach_state) (dwfl, elf, pid,
				      &sample_thread_callbacks, sample_arg))
      return -1;

  /* Now that Dwfl is attached, we can access its Ebl: */
  Dwfl_Process *process = dwfl->process;
  Ebl *ebl = process->ebl;
  sample_arg->base_addr = ebl_sample_base_addr(ebl, regs, n_regs,
					       perf_regs_mask, abi);
  sample_arg->pc = ebl_sample_pc(ebl, regs, n_regs,
				 perf_regs_mask, abi);

  return INTUSE(dwfl_getthread_frames) (dwfl, tid, callback, arg);
}
