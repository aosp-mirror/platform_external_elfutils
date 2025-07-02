/* Common pieces for handling registers in a linux perf_events sample.
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

static Dwarf_Word
perf_sample_find_reg (const Dwarf_Word *regs, uint32_t n_regs,
		      uint64_t regs_mask,
		      int target)
{
  int j, k; uint64_t bit;
  for (j = 0, k = 0, bit = 1; k < PERF_REG_X86_64_MAX; k++, bit <<= 1)
    {
      if (bit & regs_mask) {
	if (n_regs <= (uint32_t) j)
	  return 0; /* regs_mask count doesn't match n_regs */
	if (k == target)
	  return regs[j];
	if (k > target)
	  return 0; /* regs_mask doesn't include desired reg */
	j++;
      }
    }
  return 0;
}
