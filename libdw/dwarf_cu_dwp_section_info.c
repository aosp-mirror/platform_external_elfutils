/* Read DWARF package file index sections.
   Copyright (c) 2023 Meta Platforms, Inc. and affiliates.
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

#include "libdwP.h"

static Dwarf_Package_Index *
__libdw_read_package_index (Dwarf *dbg, bool tu)
{
  Elf_Data *data;
  if (tu)
    data = dbg->sectiondata[IDX_debug_tu_index];
  else
    data = dbg->sectiondata[IDX_debug_cu_index];

  /* We need at least 16 bytes for the header.  */
  if (data == NULL || data->d_size < 16)
    {
    invalid:
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return NULL;
    }

  const unsigned char *datap = data->d_buf;
  const unsigned char *endp = datap + data->d_size;
  uint16_t version;
  /* In GNU DebugFission for DWARF 4, the version is 2 as a uword.  In the
     standardized DWARF 5 format, it is a uhalf followed by a padding uhalf.
     Check for both.  */
  if (read_4ubyte_unaligned (dbg, datap) == 2)
    version = 2;
  else
    {
      version = read_2ubyte_unaligned (dbg, datap);
      if (version != 5)
	{
	  __libdw_seterrno (DWARF_E_VERSION);
	  return NULL;
	}
    }
  datap += 4;
  uint32_t section_count = read_4ubyte_unaligned_inc (dbg, datap);
  uint32_t unit_count = read_4ubyte_unaligned_inc (dbg, datap);
  uint32_t slot_count = read_4ubyte_unaligned_inc (dbg, datap);

  /* The specification has a stricter requirement that
     slot_count > 3 * unit_count / 2, but this is enough for us.  */
  if (slot_count < unit_count)
    goto invalid;

  /* After the header, the section must contain:

       8 byte signature per hash table slot
     + 4 byte index per hash table slot
     + Section offset table with 1 header row, 1 row per unit, 1 column per
       section, 4 bytes per field
     + Section size table with 1 row per unit, 1 column per section, 4 bytes
       per field

     We have to be careful about overflow when checking this.  */
  const unsigned char *hash_table = datap;
  if ((size_t) (endp - hash_table) < (uint64_t) slot_count * 12)
    goto invalid;
  const unsigned char *indices = hash_table + (size_t) slot_count * 8;
  const unsigned char *sections = indices + (size_t) slot_count * 4;
  if ((size_t) (endp - sections) < (uint64_t) section_count * 4)
    goto invalid;
  const unsigned char *section_offsets = sections + (size_t) section_count * 4;
  if ((uint64_t) unit_count * section_count > UINT64_MAX / 8
      || ((size_t) (endp - section_offsets)
	  < (uint64_t) unit_count * section_count * 8))
    goto invalid;
  const unsigned char *section_sizes
    = section_offsets + (uint64_t) unit_count * section_count * 4;

  Dwarf_Package_Index *index = malloc (sizeof (*index));
  if (index == NULL)
    {
      __libdw_seterrno (DWARF_E_NOMEM);
      return NULL;
    }

  index->dbg = dbg;
  /* Set absent sections to UINT32_MAX.  */
  memset (index->sections, 0xff, sizeof (index->sections));
  for (size_t i = 0; i < section_count; i++)
    {
      uint32_t section = read_4ubyte_unaligned (dbg, sections + i * 4);
      /* 2 is DW_SECT_TYPES in version 2 and reserved in version 5.  We ignore
         it for version 5.
	 5 is DW_SECT_LOC in version 2 and DW_SECT_LOCLISTS in version 5.  We
	 use the same index for both.
	 7 is DW_SECT_MACINFO in version 2 and DW_SECT_MACRO in version 5.  We
	 use the same index for both.
	 8 is DW_SECT_MACRO in version 2 and DW_SECT_RNGLISTS in version 5.  We
	 use the same index for version 2's DW_SECT_MACRO as version 2's
	 DW_SECT_MACINFO/version 5's DW_SECT_MACRO.
	 We ignore unknown sections.  */
      if (section == 0)
	continue;
      if (version == 2)
	{
	  if (section > 8)
	    continue;
	  else if (section == 8)
	    section = DW_SECT_MACRO;
	}
      else if (section == 2
	       || (section
		   > sizeof (index->sections) / sizeof (index->sections[0])))
	continue;
      index->sections[section - 1] = i;
    }

  /* DW_SECT_INFO (or DW_SECT_TYPES for DWARF 4 type units) and DW_SECT_ABBREV
     are required.  */
  if (((!tu || dbg->sectiondata[IDX_debug_types] == NULL)
       && index->sections[DW_SECT_INFO - 1] == UINT32_MAX)
      || (tu && dbg->sectiondata[IDX_debug_types] != NULL
	  && index->sections[DW_SECT_TYPES - 1] == UINT32_MAX)
      || index->sections[DW_SECT_ABBREV - 1] == UINT32_MAX)
    {
      free (index);
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return NULL;
    }

  index->section_count = section_count;
  index->unit_count = unit_count;
  index->slot_count = slot_count;
  index->last_unit_found = 0;
  index->hash_table = hash_table;
  index->indices = indices;
  index->section_offsets = section_offsets;
  index->section_sizes = section_sizes;

  return index;
}

static Dwarf_Package_Index *
__libdw_package_index (Dwarf *dbg, bool tu)
{
  if (tu && dbg->tu_index != NULL)
    return dbg->tu_index;
  else if (!tu && dbg->cu_index != NULL)
    return dbg->cu_index;

  Dwarf_Package_Index *index = __libdw_read_package_index (dbg, tu);
  if (index == NULL)
    return NULL;

  if (tu)
    dbg->tu_index = index;
  else
    dbg->cu_index = index;
  return index;
}

static int
__libdw_dwp_unit_row (Dwarf_Package_Index *index, uint64_t unit_id,
		      uint32_t *unit_rowp)
{
  if (index == NULL)
    return -1;

  uint32_t hash = unit_id;
  uint32_t hash2 = (unit_id >> 32) | 1;
  /* Only check each slot once.  */
  for (uint32_t n = index->slot_count; n-- > 0; )
    {
      size_t slot = hash & (index->slot_count - 1);
      uint64_t sig = read_8ubyte_unaligned (index->dbg,
					    index->hash_table + slot * 8);
      if (sig == unit_id)
	{
	  uint32_t row = read_4ubyte_unaligned (index->dbg,
						index->indices + slot * 4);
	  if (row > index->unit_count)
	    {
	      __libdw_seterrno (DWARF_E_INVALID_DWARF);
	      return -1;
	    }
	  *unit_rowp = row;
	  return 0;
	}
      else if (sig == 0
	       && read_4ubyte_unaligned (index->dbg,
					 index->indices + slot * 4) == 0)
	break;
      hash += hash2;
    }
  *unit_rowp = 0;
  return 0;
}

static int
__libdw_dwp_section_info (Dwarf_Package_Index *index, uint32_t unit_row,
			  unsigned int section, Dwarf_Off *offsetp,
			  Dwarf_Off *sizep)
{
  if (index == NULL)
    return -1;
  if (unit_row == 0)
    {
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return -1;
    }
  if (index->sections[section - 1] == UINT32_MAX)
    {
      if (offsetp != NULL)
	*offsetp = 0;
      if (sizep != NULL)
	*sizep = 0;
      return 0;
    }
  size_t i = (size_t)(unit_row - 1) * index->section_count
	     + index->sections[section - 1];
  if (offsetp != NULL)
    *offsetp = read_4ubyte_unaligned (index->dbg,
				      index->section_offsets + i * 4);
  if (sizep != NULL)
    *sizep = read_4ubyte_unaligned (index->dbg,
				    index->section_sizes + i * 4);
  return 0;
}

int
internal_function
__libdw_dwp_find_unit (Dwarf *dbg, bool debug_types, Dwarf_Off off,
		       uint16_t version, uint8_t unit_type, uint64_t unit_id8,
		       uint32_t *unit_rowp, Dwarf_Off *abbrev_offsetp)
{
  if (version >= 5
      && unit_type != DW_UT_split_compile && unit_type != DW_UT_split_type)
    {
    not_dwp:
      *unit_rowp = 0;
      *abbrev_offsetp = 0;
      return 0;
    }
  bool tu = unit_type == DW_UT_split_type || debug_types;
  if (dbg->sectiondata[tu ? IDX_debug_tu_index : IDX_debug_cu_index] == NULL)
    goto not_dwp;
  Dwarf_Package_Index *index = __libdw_package_index (dbg, tu);
  if (index == NULL)
    return -1;

  /* This is always called for ascending offsets.  The most obvious way for a
     producer to generate the section offset table is sorted by offset; both
     GNU dwp and llvm-dwp do this.  In this common case, we can avoid the full
     lookup.  */
  if (index->last_unit_found < index->unit_count)
    {
      Dwarf_Off offset, size;
      if (__libdw_dwp_section_info (index, index->last_unit_found + 1,
				    debug_types ? DW_SECT_TYPES : DW_SECT_INFO,
				    &offset, &size) != 0)
	return -1;
      if (offset <= off && off - offset < size)
	{
	  *unit_rowp = ++index->last_unit_found;
	  goto done;
	}
      else
	/* The units are not sorted. Don't try again.  */
	index->last_unit_found = index->unit_count;
    }

  if (version >= 5 || debug_types)
    {
      /* In DWARF 5 and in type units, the unit signature is available in the
         unit header.  */
      if (__libdw_dwp_unit_row (index, unit_id8, unit_rowp) != 0)
	return -1;
    }
  else
    {
      /* In DWARF 4 compilation units, the unit signature is an attribute.  We
	 can't parse attributes in the split unit until we get the abbreviation
	 table offset from the package index, which is a chicken-and-egg
	 problem.  We could get the signature from the skeleton unit, but that
	 may not be available.

	 Instead, we resort to a linear scan through the section offset table.
	 Finding all units is therefore quadratic in the number of units.
	 However, this will likely never be needed in practice because of the
	 sorted fast path above.  If this ceases to be the case, we can try to
	 plumb through the skeleton unit's signature when it is available, or
	 build a sorted lookup table for binary search.  */
      if (index->sections[DW_SECT_INFO - 1] == UINT32_MAX)
	{
	  __libdw_seterrno (DWARF_E_INVALID_DWARF);
	  return -1;
	}
      for (uint32_t i = 0; i < index->unit_count; i++)
	{
	  Dwarf_Off offset, size;
	  __libdw_dwp_section_info (index, i + 1, DW_SECT_INFO, &offset,
				    &size);
	  if (offset <= off && off - offset < size)
	    {
	      *unit_rowp = i + 1;
	      goto done;
	    }
	}
      __libdw_seterrno (DWARF_E_INVALID_DWARF);
      return -1;
    }

 done:
  return __libdw_dwp_section_info (index, *unit_rowp, DW_SECT_ABBREV,
				   abbrev_offsetp, NULL);
}

int
dwarf_cu_dwp_section_info (Dwarf_CU *cu, unsigned int section,
			   Dwarf_Off *offsetp, Dwarf_Off *sizep)
{
  if (cu == NULL)
    return -1;
  if (section < DW_SECT_INFO || section > DW_SECT_RNGLISTS)
    {
      __libdw_seterrno (DWARF_E_UNKNOWN_SECTION);
      return -1;
    }
  if (cu->dwp_row == 0)
    {
      if (offsetp != NULL)
	*offsetp = 0;
      if (sizep != NULL)
	*sizep = 0;
      return 0;
    }
  else
    {
      Dwarf_Package_Index *index
	= cu->unit_type == DW_UT_split_compile
	? cu->dbg->cu_index : cu->dbg->tu_index;
      return __libdw_dwp_section_info (index, cu->dwp_row, section, offsetp,
				       sizep);
    }
}
INTDEF(dwarf_cu_dwp_section_info)
