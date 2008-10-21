/* Find line information for address.
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

#include "libdwP.h"


Dwarf_Line *
dwarf_getsrc_die (Dwarf_Die *cudie, Dwarf_Addr addr)
{
  Dwarf_Lines *lines;
  size_t nlines;

  if (dwarf_getsrclines (cudie, &lines, &nlines) != 0)
    return NULL;

  if (addr >= lines->info[0].addr )
    for (size_t n = 1; n < nlines; ++n)
      if (addr < lines->info[n].addr)
	return &lines->info[n - 1];

  __libdw_seterrno (DWARF_E_ADDR_OUTOFRANGE);
  return NULL;
}
