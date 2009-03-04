/* Return location expression list.
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

#include <dwarf.h>
#include <search.h>
#include <stdlib.h>

#include <libdwP.h>


struct loclist
{
  uint8_t atom;
  Dwarf_Word number;
  Dwarf_Word number2;
  Dwarf_Word offset;
  struct loclist *next;
};


static int
loc_compare (const void *p1, const void *p2)
{
  const struct loc_s *l1 = (const struct loc_s *) p1;
  const struct loc_s *l2 = (const struct loc_s *) p2;

  if ((uintptr_t) l1->addr < (uintptr_t) l2->addr)
    return -1;
  if ((uintptr_t) l1->addr > (uintptr_t) l2->addr)
    return 1;

  return 0;
}


int
dwarf_getloclist (attr, llbuf, listlen)
     Dwarf_Attribute *attr;
     Dwarf_Loc **llbuf;
     size_t *listlen;
{
  /* Must by one of the attribute listed below.  */
  if (attr->code != DW_AT_location
      && attr->code != DW_AT_data_member_location
      && attr->code != DW_AT_vtable_elem_location
      && attr->code != DW_AT_string_length
      && attr->code != DW_AT_use_location
      && attr->code != DW_AT_return_addr)
    {
      __libdw_seterrno (DWARF_E_NO_LOCLIST);
      return -1;
    }

  struct Dwarf_CU *cu = attr->cu;
  Dwarf *dbg = cu->dbg;

  /* Must have the form data4 or data8 which act as an offset.  */
  Dwarf_Block block;
  if (dwarf_formblock (attr, &block) != 0)
    return -1;

  /* Check whether we already looked at this list.  */
  struct loc_s fake = { .addr = block.data };
  struct loc_s **found = tfind (&fake, &cu->locs, loc_compare);
  if (found != NULL)
    {
      /* We already saw it.  */
      *llbuf = (*found)->loc;
      *listlen = (*found)->nloc;

      return 0;
    }

  unsigned char *data = block.data;
  unsigned char *const end_data = data + block.length;

  struct loclist *loclist = NULL;
  unsigned int n = 0;
  /* Decode the opcodes.  It is possible in some situations to have a
     block of size zero.  */
  while (data < end_data)
    {
      struct loclist *newloc;
      newloc = (struct loclist *) alloca (sizeof (struct loclist));
      newloc->number = 0;
      newloc->number2 = 0;
      newloc->offset = data - block.data;
      newloc->next = loclist;
      loclist = newloc;
      ++n;

      switch ((newloc->atom = *data++))
	{
	case DW_OP_addr:
	  /* Address, depends on address size of CU.  */
	  if (cu->address_size == 4)
	    {
	      if (unlikely (data + 4 > end_data))
		{
		invalid:
		  __libdw_seterrno (DWARF_E_INVALID_DWARF);
		  return -1;
		}

	      newloc->number = read_4ubyte_unaligned_inc (dbg, data);
	    }
	  else
	    {
	      if (unlikely (data + 8 > end_data))
		goto invalid;

	      newloc->number = read_8ubyte_unaligned_inc (dbg, data);
	    }
	  break;

	case DW_OP_deref:
	case DW_OP_dup:
	case DW_OP_drop:
	case DW_OP_over:
	case DW_OP_swap:
	case DW_OP_rot:
	case DW_OP_xderef:
	case DW_OP_abs:
	case DW_OP_and:
	case DW_OP_div:
	case DW_OP_minus:
	case DW_OP_mod:
	case DW_OP_mul:
	case DW_OP_neg:
	case DW_OP_not:
	case DW_OP_or:
	case DW_OP_plus:
	case DW_OP_shl:
	case DW_OP_shr:
	case DW_OP_shra:
	case DW_OP_xor:
	case DW_OP_eq:
	case DW_OP_ge:
	case DW_OP_gt:
	case DW_OP_le:
	case DW_OP_lt:
	case DW_OP_ne:
	case DW_OP_lit0 ... DW_OP_lit31:
	case DW_OP_reg0 ... DW_OP_reg31:
	case DW_OP_nop:
	case DW_OP_push_object_address:
	case DW_OP_call_ref:
	  /* No operand.  */
	  break;

	case DW_OP_const1u:
	case DW_OP_pick:
	case DW_OP_deref_size:
	case DW_OP_xderef_size:
	  if (unlikely (data >= end_data))
	    goto invalid;

	  newloc->number = *data++;
	  break;

	case DW_OP_const1s:
	  if (unlikely (data >= end_data))
	    goto invalid;

	  newloc->number = *((int8_t *) data);
	  ++data;
	  break;

	case DW_OP_const2u:
	  if (unlikely (data + 2 > end_data))
	    goto invalid;

	  newloc->number = read_2ubyte_unaligned_inc (dbg, data);
	  break;

	case DW_OP_const2s:
	case DW_OP_skip:
	case DW_OP_bra:
	case DW_OP_call2:
	  if (unlikely (data + 2 > end_data))
	    goto invalid;

	  newloc->number = read_2sbyte_unaligned_inc (dbg, data);
	  break;

	case DW_OP_const4u:
	  if (unlikely (data + 4 > end_data))
	    goto invalid;

	  newloc->number = read_4ubyte_unaligned_inc (dbg, data);
	  break;

	case DW_OP_const4s:
	case DW_OP_call4:
	  if (unlikely (data + 4 > end_data))
	    goto invalid;

	  newloc->number = read_4sbyte_unaligned_inc (dbg, data);
	  break;

	case DW_OP_const8u:
	  if (unlikely (data + 8 > end_data))
	    goto invalid;

	  newloc->number = read_8ubyte_unaligned_inc (dbg, data);
	  break;

	case DW_OP_const8s:
	  if (unlikely (data + 8 > end_data))
	    goto invalid;

	  newloc->number = read_8sbyte_unaligned_inc (dbg, data);
	  break;

	case DW_OP_constu:
	case DW_OP_plus_uconst:
	case DW_OP_regx:
	case DW_OP_piece:
	  /* XXX Check size.  */
	  get_uleb128 (newloc->number, data);
	  break;

	case DW_OP_consts:
	case DW_OP_breg0 ... DW_OP_breg31:
	case DW_OP_fbreg:
	  /* XXX Check size.  */
	  get_sleb128 (newloc->number, data);
	  break;

	case DW_OP_bregx:
	  /* XXX Check size.  */
	  get_uleb128 (newloc->number, data);
	  get_sleb128 (newloc->number2, data);
	  break;

	default:
	  goto invalid;
	}
    }

  if (unlikely (n == 0))
    {
      /* This is not allowed.

	 XXX Is it?  */
      goto invalid;
    }

  /* Allocate the array.  */
  Dwarf_Loc *result = libdw_alloc (dbg, Dwarf_Loc, sizeof (Dwarf_Loc), n);

  /* Store the result.  */
  *llbuf = result;
  *listlen = n;

  do
    {
      /* We populate the array from the back since the list is
         backwards.  */
      --n;
      result[n].atom = loclist->atom;
      result[n].number = loclist->number;
      result[n].number2 = loclist->number2;
      result[n].offset = loclist->offset;

      loclist = loclist->next;
    }
  while (n > 0);

  /* Insert a record in the search tree so that we can find it again
     later.  */
  struct loc_s *newp = libdw_alloc (dbg, struct loc_s, sizeof (struct loc_s),
				    1);
  newp->addr = block.data;
  newp->loc = result;
  newp->nloc = *listlen;
  (void) tsearch (newp, &cu->locs, loc_compare);

  /* We did it.  */
  return 0;
}
