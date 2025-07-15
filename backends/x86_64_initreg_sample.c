/* Populate process registers from a linux perf_events sample.
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

#include <stdlib.h>
#if defined(__x86_64__) && defined(__linux__)
# include <linux/perf_event.h>
# include <asm/perf_regs.h>
#endif

#define BACKEND x86_64_
#include "libebl_CPU.h"
#include "libebl_PERF_FLAGS.h"
#if defined(__x86_64__) && defined(__linux__)
# include "linux-perf-regs.c"
# include "x86_initreg_sample.c"
#endif

/* Register ordering cf. linux arch/x86/include/uapi/asm/perf_regs.h,
   enum perf_event_x86_regs: */
Dwarf_Word
x86_64_sample_base_addr (const Dwarf_Word *regs, uint32_t n_regs,
			 uint64_t regs_mask,
			 /* XXX hypothetically needed if abi varies
			    between samples in the same process;
			    not needed on x86*/
			 uint32_t abi __attribute__((unused)))
{
#if !defined(__x86_64__) || !defined(__linux__)
  (void)regs;
  (void)n_regs;
  (void)regs_mask;
  return 0;
#else /* __x86_64__ */
  return perf_sample_find_reg (regs, n_regs, regs_mask,
			       7 /* index into perf_event_x86_regs */);
#endif
}

Dwarf_Word
x86_64_sample_pc (const Dwarf_Word *regs, uint32_t n_regs,
		  uint64_t regs_mask,
		  uint32_t abi __attribute__((unused)))
{
#if !defined(__x86_64__) || !defined(__linux__)
  (void)regs;
  (void)n_regs;
  (void)regs_mask;
  return 0;
#else /* __x86_64__ */
  return perf_sample_find_reg (regs, n_regs, regs_mask,
			       8 /* index into perf_event_x86_regs */);
#endif
}

bool
x86_64_set_initial_registers_sample (const Dwarf_Word *regs, uint32_t n_regs,
				     uint64_t regs_mask, uint32_t abi,
				     ebl_tid_registers_t *setfunc,
				     void *arg)
{
#if !defined(__x86_64__) || !defined(__linux__)
  (void)regs;
  (void)n_regs;
  (void)regs_mask;
  (void)abi;
  (void)setfunc;
  (void)arg;
  return false;
#else /* __x86_64__ */
  Dwarf_Word dwarf_regs[17];
  if (!x86_set_initial_registers_sample (regs, n_regs, regs_mask,
					 abi, dwarf_regs, 9))
    return false;
  return setfunc (0, 17, dwarf_regs, arg);
#endif
}

