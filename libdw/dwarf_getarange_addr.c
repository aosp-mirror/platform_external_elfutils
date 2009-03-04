/* Get address range which includes given address.
   Copyright (C) 2004 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2004.

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

#include <libdwP.h>


Dwarf_Arange *
dwarf_getarange_addr (aranges, addr)
     Dwarf_Aranges *aranges;
     Dwarf_Addr addr;
{
  if (aranges == NULL)
    return NULL;

  for (size_t cnt = 0; cnt < aranges->naranges; ++cnt)
    if (aranges->info[cnt].addr <= addr
        && addr < aranges->info[cnt].addr + aranges->info[cnt].length)
      return &aranges->info[cnt];

  __libdw_seterrno (DWARF_E_NO_MATCH);
  return NULL;
}
