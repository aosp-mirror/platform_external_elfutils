/* Return list address ranges.
   Copyright (C) 2000, 2001, 2002, 2004 Red Hat, Inc.
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

#include <stdlib.h>

#include <libdwP.h>


struct arangelist
{
  Dwarf_Arange arange;
  struct arangelist *next;
};


int
dwarf_getaranges (dbg, aranges, naranges)
     Dwarf *dbg;
     Dwarf_Aranges **aranges;
     size_t *naranges;
{
  if (dbg == NULL)
    return -1;

  if (dbg->aranges != NULL)
    {
      *aranges = dbg->aranges;
      if (naranges != NULL)
	*naranges = dbg->aranges->naranges;
      return 0;
    }

  if (dbg->sectiondata[IDX_debug_aranges]->d_buf == NULL)
    return -1;

  struct arangelist *arangelist = NULL;
  unsigned int narangelist = 0;

  const char *readp
    = (const char *) dbg->sectiondata[IDX_debug_aranges]->d_buf;
  const char *readendp = readp + dbg->sectiondata[IDX_debug_aranges]->d_size;

  while (readp < readendp)
    {
      const char *hdrstart = readp;

      /* Each entry starts with a header:

	 1. A 4-byte or 12-byte length containing the length of the
	 set of entries for this compilation unit, not including the
	 length field itself. [...]

	 2. A 2-byte version identifier containing the value 2 for
	 DWARF Version 2.1.

	 3. A 4-byte or 8-byte offset into the .debug_info section. [...]

	 4. A 1-byte unsigned integer containing the size in bytes of
	 an address (or the offset portion of an address for segmented
	 addressing) on the target system.

	 5. A 1-byte unsigned integer containing the size in bytes of
	 a segment descriptor on the target system.  */
      Dwarf_Word length = read_4ubyte_unaligned_inc (dbg, readp);
      unsigned int length_bytes = 4;
      if (length == 0xffffffff)
	{
	  length = read_8ubyte_unaligned_inc (dbg, readp);
	  length_bytes = 8;
	}

      unsigned int version = read_2ubyte_unaligned_inc (dbg, readp);
      if (version != 2)
	{
	invalid:
	  __libdw_seterrno (DWARF_E_INVALID_DWARF);
	  return -1;
	}

      Dwarf_Word offset;
      if (length_bytes == 4)
	offset = read_4ubyte_unaligned_inc (dbg, readp);
      else
	offset = read_8ubyte_unaligned_inc (dbg, readp);

      unsigned int address_size = *readp++;
      if (address_size != 4 && address_size != 8)
	goto invalid;

      /* Ignore the segment size value.  */
      // XXX Really?
      (void) *readp++;

      /* Round the address to the next multiple of 2*address_size.  */
      readp += ((2 * address_size - ((readp - hdrstart) % (2 * address_size)))
		% (2 * address_size));

      //arange_info->offset = offset;

      while (1)
	{
	  Dwarf_Word range_address;
	  Dwarf_Word range_length;

	  if (address_size == 4)
	    {
	      range_address = read_4ubyte_unaligned_inc (dbg, readp);
	      range_length = read_4ubyte_unaligned_inc (dbg, readp);
	    }
	  else
	    {
	      range_address = read_8ubyte_unaligned_inc (dbg, readp);
	      range_length = read_8ubyte_unaligned_inc (dbg, readp);
	    }

	  /* Two zero values mark the end.  */
	  if (range_address == 0 && range_length == 0)
	    break;

	  struct arangelist *new_arange =
	    (struct arangelist *) alloca (sizeof (struct arangelist));

	  new_arange->arange.addr = range_address;
	  new_arange->arange.length = range_length;

	  /* We store the actual CU DIE offset, not the CU header offset.  */
	  const char *cu_header = (dbg->sectiondata[IDX_debug_info]->d_buf
				   + offset);
	  unsigned int offset_size;
	  if (read_4ubyte_unaligned_noncvt (cu_header) == 0xffffffff)
	    offset_size = 8;
	  else
	    offset_size = 4;
	  new_arange->arange.offset = offset + 3 * offset_size - 4 + 3;

	  new_arange->next = arangelist;
	  arangelist = new_arange;
	  ++narangelist;
	}
    }

  if (narangelist == 0)
    {
      if (naranges != NULL)
	*naranges = 0;
      *aranges = NULL;
      return 0;
    }

  /* Allocate the array for the result.  */
  if (naranges != NULL)
    *naranges = narangelist;
  *aranges = libdw_alloc (dbg, Dwarf_Aranges,
			  sizeof (Dwarf_Aranges)
			  + narangelist * sizeof (Dwarf_Arange), 1);

  (*aranges)->dbg = dbg;
  (*aranges)->naranges = narangelist;

  while (narangelist-- > 0)
    {
      (*aranges)->info[narangelist] = arangelist->arange;
      arangelist = arangelist->next;
    }

  dbg->aranges = *aranges;

  return 0;
}
