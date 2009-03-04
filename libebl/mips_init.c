/* Initialization of MIPS specific backend library.
   Copyright (C) 2000, 2001, 2002 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2000.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libebl_mips.h>


int
mips_init (elf, machine, eh, ehlen)
     Elf *elf;
     GElf_Half machine;
     Ebl *eh;
     size_t ehlen;
{
  /* Check whether the Elf_BH object has a sufficent size.  */
  if (ehlen < sizeof (Ebl))
    return 1;

  /* We handle it.  */
  if (machine != EM_MIPS)
    eh->name = "MIPS R3000 big-endian";
  else
    eh->name = "MIPS R3000 little-endian";

  eh->reloc_type_name = mips_reloc_type_name;
  eh->segment_type_name = mips_segment_type_name;
  eh->section_type_name = mips_section_type_name;
  eh->machine_flag_name = mips_machine_flag_name;
  eh->dynamic_tag_name = mips_dynamic_tag_name;
  eh->destr = mips_destr;

  return 0;
}
