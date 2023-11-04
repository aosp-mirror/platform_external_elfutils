/* LoongArch specific symbolic name handling.
   Copyright (C) 2022 Hengqi Chen
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

#include <assert.h>
#include <elf.h>
#include <stddef.h>
#include <string.h>

#define BACKEND loongarch_
#include "libebl_CPU.h"


/* Check for the simple reloc types.  */
Elf_Type
loongarch_reloc_simple_type (Ebl *ebl __attribute__ ((unused)), int type,
			     int *addsub)
{
  switch (type)
    {
    case R_LARCH_32:
      return ELF_T_WORD;
    case R_LARCH_64:
      return ELF_T_XWORD;
    case R_LARCH_ADD16:
      *addsub = 1;
      return ELF_T_HALF;
    case R_LARCH_ADD32:
      *addsub = 1;
      return ELF_T_WORD;
    case R_LARCH_ADD64:
      *addsub = 1;
      return ELF_T_XWORD;
    case R_LARCH_SUB16:
      *addsub = -1;
      return ELF_T_HALF;
    case R_LARCH_SUB32:
      *addsub = -1;
      return ELF_T_WORD;
    case R_LARCH_SUB64:
      *addsub = -1;
      return ELF_T_XWORD;
    default:
      return ELF_T_NUM;
    }
}
