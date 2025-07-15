/* Populate process Dwfl_Frame from perf_events sample.

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

#include <libeblP.h>
#include <assert.h>

Dwarf_Word
ebl_sample_base_addr (Ebl *ebl,
                      const Dwarf_Word *regs, uint32_t n_regs,
		      uint64_t regs_mask, uint32_t abi)
{
  assert (ebl->sample_base_addr != NULL);
  return ebl->sample_base_addr (regs, n_regs, regs_mask, abi);
}

Dwarf_Word
ebl_sample_pc (Ebl *ebl,
	       const Dwarf_Word *regs, uint32_t n_regs,
	       uint64_t regs_mask, uint32_t abi)
{
  assert (ebl->sample_pc != NULL);
  return ebl->sample_pc (regs, n_regs, regs_mask, abi);
}

bool
ebl_set_initial_registers_sample (Ebl *ebl,
				  const Dwarf_Word *regs, uint32_t n_regs,
				  uint64_t regs_mask, uint32_t abi,
				  ebl_tid_registers_t *setfunc,
				  void *arg)
{
  /* If set_initial_registers_sample is unsupported then PERF_FRAME_REGS_MASK is zero.  */
  assert (ebl->set_initial_registers_sample != NULL);
  return ebl->set_initial_registers_sample (regs, n_regs, regs_mask, abi, setfunc, arg);
}

uint64_t
ebl_perf_frame_regs_mask (Ebl *ebl)
{
  /* ebl is declared NN */
  return ebl->perf_frame_regs_mask;
}
