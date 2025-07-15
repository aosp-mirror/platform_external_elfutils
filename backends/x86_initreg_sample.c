/* x86 linux perf_events register handling, pieces common to x86-64 and i386.
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

static bool
x86_set_initial_registers_sample (const Dwarf_Word *regs, uint32_t n_regs,
				  uint64_t regs_mask, uint32_t abi,
				  Dwarf_Word *dwarf_regs, int expected_regs)
{
#if (!defined __i386__ && !defined __x86_64__) || !defined(__linux__)
  return false;
#else /* __i386__ || __x86_64__ */
  /* The following facts are needed to translate x86 registers correctly:
     - perf register order seen in linux arch/x86/include/uapi/asm/perf_regs.h
       The registers array is built in the same order as the enum!
       (See the code in tools/perf/util/intel-pt.c intel_pt_add_gp_regs().)
     - EBL PERF_FRAME_REGS_MASK specifies all registers except segment and
       flags.  However, regs_mask might be a different set of registers.
       Again, regs_mask bits are in asm/perf_regs.h enum order.
     - dwarf register order seen in elfutils backends/{x86_64,i386}_initreg.c
       (matching pt_regs struct in linux arch/x86/include/asm/ptrace.h)
       and it's a fairly different register order!

     For comparison, you can study codereview.qt-project.org/gitweb?p=qt-creator/perfparser.git;a=blob;f=app/perfregisterinfo.cpp;hb=HEAD
     and follow the code which uses those tables of magic numbers.
     But it's better to follow original sources of truth for this.  */

  bool is_abi32 = (abi == PERF_SAMPLE_REGS_ABI_32);

  /* Locations of dwarf_regs in the perf_event_x86_regs enum order,
     not the regs[i] array (which will include a subset of the regs): */
  static const int regs_i386[] = {0, 2, 3, 1, 7/*sp*/, 6, 4, 5, 8/*ip*/};
  static const int regs_x86_64[] = {0, 3, 2, 1, 4, 5, 6, 7/*sp*/,
				    16/*r8 after flags+segment*/, 17, 18, 19, 20, 21, 22, 23,
				    8/*ip*/};
  const int *dwarf_to_perf = is_abi32 ? regs_i386 : regs_x86_64;

  /* Locations of perf_regs in the regs[] array, according to regs_mask: */
  int perf_to_regs[PERF_REG_X86_64_MAX];
  uint64_t expected_mask = is_abi32 ? PERF_FRAME_REGISTERS_I386 : PERF_FRAME_REGISTERS_X86_64;
  int j, k; uint64_t bit;
  /* TODO: Is it worth caching this perf_to_regs computation as long
     as regs_mask is kept the same across repeated calls? */
  for (j = 0, k = 0, bit = 1; k < PERF_REG_X86_64_MAX; k++, bit <<= 1)
    {
      if ((bit & expected_mask) && (bit & regs_mask)) {
	if (n_regs <= (uint32_t)j)
	  return false; /* regs_mask count doesn't match n_regs */
	perf_to_regs[k] = j;
	j++;
      } else {
	perf_to_regs[k] = -1;
      }
    }

  for (int i = 0; i < expected_regs; i++)
    {
      k = dwarf_to_perf[i];
      j = perf_to_regs[k];
      if (j < 0) continue;
      if (n_regs <= (uint32_t)j) continue;
      dwarf_regs[i] = regs[j];
    }
  return true;
#endif /* __i386__ || __x86_64__ */
}
