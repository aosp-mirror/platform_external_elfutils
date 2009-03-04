/* Interfaces for libdw.
   Copyright (C) 2002, 2004 Red Hat, Inc.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifndef _LIBDW_H
#define _LIBDW_H	1

#include <gelf.h>
#include <stdbool.h>
#include <stddef.h>


#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
# define __nonnull_attribute__(...) __attribute__ ((__nonnull__ (__VA_ARGS__)))
#else
# define __nonnull_attribute__(args...)
#endif


/* Mode for the session.  */
typedef enum
  {
    DWARF_C_READ,		/* Read .. */
    DWARF_C_RDWR,		/* Read and write .. */
    DWARF_C_WRITE,		/* Write .. */
  }
Dwarf_Cmd;


/* Callback results.  */
enum
{
  DWARF_CB_OK = 0,
  DWARF_CB_ABORT
};


/* Error values.  */
enum
  {
    DW_TAG_invalid = 0
#define DW_TAG_invalid	DW_TAG_invalid
  };


/* Type for offset in DWARF file.  */
typedef GElf_Off Dwarf_Off;

/* Type for address in DWARF file.  */
typedef GElf_Addr Dwarf_Addr;

/* Integer types.  Big enough to hold any numeric value.  */
typedef GElf_Xword Dwarf_Word;
typedef GElf_Sxword Dwarf_Sword;
/* For the times we know we do not need that much.  */
typedef GElf_Half Dwarf_Half;


/* DWARF abbreviation record.  */
typedef struct Dwarf_Abbrev Dwarf_Abbrev;

/* Source code line information for CU.  */
typedef struct Dwarf_Lines_s Dwarf_Lines;

/* One source code line information.  */
typedef struct Dwarf_Line_s Dwarf_Line;

/* Source file information.  */
typedef struct Dwarf_Files_s Dwarf_Files;

/* One address range record.  */
typedef struct Dwarf_Arange_s Dwarf_Arange;

/* Address ranges of a file.  */
typedef struct Dwarf_Aranges_s Dwarf_Aranges;

/* CU representation.  */
struct Dwarf_CU;

/* Attribute representation.  */
typedef struct
{
  unsigned int code;
  unsigned int form;
  unsigned char *valp;
  struct Dwarf_CU *cu;
} Dwarf_Attribute;


/* Data block representation.  */
typedef struct
{
  Dwarf_Word length;
  unsigned char *data;
} Dwarf_Block;


/* Macro information.  */
typedef struct
{
  unsigned int opcode;
  Dwarf_Word param1;
  union
  {
    Dwarf_Word u;
    const char *s;
  } param2;
} Dwarf_Macro;


/* DIE information.  */
typedef struct
{
  /* The offset can be computed from the address.  */
  void *addr;
  struct Dwarf_CU *cu;
  Dwarf_Abbrev *abbrev;
  // XXX We'll see what other information will be needed.
} Dwarf_Die;

/* Returned to show the last DIE has be returned.  */
#define DWARF_END_DIE ((Dwarf_Die *) -1l)


/* Global symbol information.  */
typedef struct
{
  Dwarf_Off cu_offset;
  Dwarf_Off die_offset;
  const char *name;
} Dwarf_Global;


// XXX It remains to be seen whether the next two need to be exported.
/* Location record.  */
typedef struct
{
  uint8_t atom;			/* Operation */
  Dwarf_Word number;		/* Operand */
  Dwarf_Word number2;		/* Possible second operand */
  Dwarf_Word offset;		/* Offset in location expression */
} Dwarf_Loc;


/* Handle for debug sessions.  */
typedef struct Dwarf Dwarf;


/* Out-Of-Memory handler.  */
typedef void (*__attribute__ ((noreturn)) Dwarf_OOM) (void);


/* Create a handle for a new debug session.  */
extern Dwarf *dwarf_begin (int fildes, Dwarf_Cmd cmd);

/* Create a handle for a new debug session for an ELF file.  */
extern Dwarf *dwarf_begin_elf (Elf *elf, Dwarf_Cmd cmd, Elf_Scn *scngrp);

/* Retrieve ELF descriptor used for DWARF access.  */
extern Elf *dwarf_getelf (Dwarf *dwarf);

/* Release debugging handling context.  */
extern int dwarf_end (Dwarf *dwarf);


/* Get the data block for the .debug_info section.  */
extern Elf_Data *dwarf_getscn_info (Dwarf *dwarf);

/* Read the header for the DWARF CU header.  */
extern int dwarf_nextcu (Dwarf *dwarf, Dwarf_Off off, Dwarf_Off *next_off,
			 size_t *header_sizep, Dwarf_Off *abbrev_offsetp,
			 uint8_t *address_sizep, uint8_t *offset_sizep)
     __nonnull_attribute__ (3);


/* Return DIE at given offset.  */
extern Dwarf_Die *dwarf_offdie (Dwarf *dbg, Dwarf_Off offset,
				Dwarf_Die *result) __nonnull_attribute__ (3);

/* Return offset of DIE.  */
extern Dwarf_Off dwarf_dieoffset (Dwarf_Die *die);

/* Return offset of DIE in CU.  */
extern Dwarf_Off dwarf_cuoffset (Dwarf_Die *die);

/* Return vhild of current DIE.  */
extern int dwarf_child (Dwarf_Die *die, Dwarf_Die *result)
     __nonnull_attribute__ (2);

/* Return sibling of given DIE.  */
extern int dwarf_siblingof (Dwarf_Die *die, Dwarf_Die *result)
     __nonnull_attribute__ (2);

/* Check whether the DIE has children.  */
extern int dwarf_haschildren (Dwarf_Die *die);

/* Get attributes of the DIE.  */
extern ptrdiff_t dwarf_getattrs (Dwarf_Die *die,
				 int (*callback) (Dwarf_Attribute *, void *),
				 void *arg, ptrdiff_t offset);

/* Return tag of given DIE.  */
extern int dwarf_tag (Dwarf_Die *die);


/* Return specific attribute of DIE.  */
extern Dwarf_Attribute *dwarf_attr (Dwarf_Die *die, unsigned int search_name,
				    Dwarf_Attribute *result)
     __nonnull_attribute__ (3);

/* Check whether given DIE has specific attribute.  */
extern int dwarf_hasattr (Dwarf_Die *die, unsigned int search_name);


/* Check whether given attribute has specific form.  */
extern int dwarf_hasform (Dwarf_Attribute *attr, unsigned int search_form);

/* Return attribute code of given attribute.  */
extern unsigned int dwarf_whatattr (Dwarf_Attribute *attr);

/* Return form code of given attribute.  */
extern unsigned int dwarf_whatform (Dwarf_Attribute *attr);


/* Return string associated with given attribute.  */
extern const char *dwarf_formstring (Dwarf_Attribute *attrp);

/* Return unsigned constant represented by attribute.  */
extern int dwarf_formudata (Dwarf_Attribute *attr, Dwarf_Word *return_uval)
     __nonnull_attribute__ (2);

/* Return signed constant represented by attribute.  */
extern int dwarf_formsdata (Dwarf_Attribute *attr, Dwarf_Sword *return_uval)
     __nonnull_attribute__ (2);

/* Return address represented by attribute.  */
extern int dwarf_formaddr (Dwarf_Attribute *attr, Dwarf_Addr *return_addr)
     __nonnull_attribute__ (2);

/* Return reference offset represented by attribute.  */
extern int dwarf_formref (Dwarf_Attribute *attr, Dwarf_Off *return_offset)
     __nonnull_attribute__ (2);

/* Return block represented by attribute.  */
extern int dwarf_formblock (Dwarf_Attribute *attr, Dwarf_Block *return_block)
     __nonnull_attribute__ (2);

/* Return flag represented by attribute.  */
extern int dwarf_formflag (Dwarf_Attribute *attr, bool *return_bool)
     __nonnull_attribute__ (2);


/* Simplified attribute value access functions.  */

/* Return string in name attribute of DIE.  */
extern const char *dwarf_diename (Dwarf_Die *die);

/* Return high PC attribute of DIE.  */
extern int dwarf_highpc (Dwarf_Die *die, Dwarf_Addr *return_addr)
     __nonnull_attribute__ (2);

/* Return low PC attribute of DIE.  */
extern int dwarf_lowpc (Dwarf_Die *die, Dwarf_Addr *return_addr)
     __nonnull_attribute__ (2);

/* Return byte size attribute of DIE.  */
extern int dwarf_bytesize (Dwarf_Die *die);

/* Return bit size attribute of DIE.  */
extern int dwarf_bitsize (Dwarf_Die *die);

/* Return bit offset attribute of DIE.  */
extern int dwarf_bitoffset (Dwarf_Die *die);

/* Return array order attribute of DIE.  */
extern int dwarf_arrayorder (Dwarf_Die *die);

/* Return source language attribute of DIE.  */
extern int dwarf_srclang (Dwarf_Die *die);


/* Get abbreviation at given offset for given DIE.  */
extern Dwarf_Abbrev *dwarf_getabbrev (Dwarf_Die *die, Dwarf_Off offset,
				      size_t *lengthp);

/* Get abbreviation at given offset in .debug_abbrev section.  */
extern int dwarf_offabbrev (Dwarf *dbg, Dwarf_Off offset, size_t *lengthp,
			    Dwarf_Abbrev *abbrevp)
     __nonnull_attribute__ (4);

/* Get abbreviation code.  */
extern unsigned int dwarf_getabbrevcode (Dwarf_Abbrev *abbrev);

/* Get abbreviation tag.  */
extern unsigned int dwarf_getabbrevtag (Dwarf_Abbrev *abbrev);

/* Return true if abbreviation is children flag set.  */
extern int dwarf_abbrevhaschildren (Dwarf_Abbrev *abbrev);

/* Get number of attributes of abbreviation.  */
extern int dwarf_getattrcnt (Dwarf_Abbrev *abbrev, size_t *attrcntp)
     __nonnull_attribute__ (2);

/* Get specific attribute of abbreviation.  */
extern int dwarf_getabbrevattr (Dwarf_Abbrev *abbrev, size_t idx,
				unsigned int *namep, unsigned int *formp,
				Dwarf_Off *offset);


/* Get string from-debug_str section.  */
extern const char *dwarf_getstring (Dwarf *dbg, Dwarf_Off offset,
				    size_t *lenp);


/* Get public symbol information.  */
extern ptrdiff_t dwarf_getpubnames (Dwarf *dbg,
				    int (*callback) (Dwarf *, Dwarf_Global *,
						     void *),
				    void *arg, ptrdiff_t offset)
     __nonnull_attribute__ (2);


/* Get source file information for CU.  */
extern int dwarf_getsrclines (Dwarf_Die *cudie, Dwarf_Lines **lines,
			      size_t *nlines) __nonnull_attribute__ (2, 3);

/* Return one of the source lines of the CU.  */
extern Dwarf_Line *dwarf_onesrcline (Dwarf_Lines *lines, size_t idx);

/* Get the file source files used in the CU.  */
extern int dwarf_getsrcfiles (Dwarf_Die *cudie, Dwarf_Files **files,
			      size_t *nfiles)
     __nonnull_attribute__ (2);


/* Get source for address in CU.  */
extern Dwarf_Line *dwarf_getsrc_die (Dwarf_Die *cudie, Dwarf_Addr addr);

/* Return line address.  */
extern int dwarf_lineaddr (Dwarf_Line *line, Dwarf_Addr *addrp);

/* Return line number.  */
extern int dwarf_lineno (Dwarf_Line *line, int *linep)
     __nonnull_attribute__ (2);

/* Return column in line.  */
extern int dwarf_linecol (Dwarf_Line *line, int *colp)
     __nonnull_attribute__ (2);

/* Return true if record is for beginning of a statement.  */
extern int dwarf_linebeginstatement (Dwarf_Line *line, bool *flagp)
     __nonnull_attribute__ (2);

/* Return true if record is for end of sequence.  */
extern int dwarf_lineendsequence (Dwarf_Line *line, bool *flagp)
     __nonnull_attribute__ (2);

/* Return true if record is for beginning of a basic block.  */
extern int dwarf_lineblock (Dwarf_Line *line, bool *flagp)
     __nonnull_attribute__ (2);

/* Return true if record is for end of prologue.  */
extern int dwarf_lineprologueend (Dwarf_Line *line, bool *flagp)
     __nonnull_attribute__ (2);

/* Return true if record is for beginning of epilogue.  */
extern int dwarf_lineepiloguebegin (Dwarf_Line *line, bool *flagp)
     __nonnull_attribute__ (2);


/* Find line information for address.  */
extern const char *dwarf_linesrc (Dwarf_Line *line,
				  Dwarf_Word *mtime, Dwarf_Word *length);

/* Return file information.  */
extern const char *dwarf_filesrc (Dwarf_Files *file, size_t idx,
				  Dwarf_Word *mtime, Dwarf_Word *length);


/* Return location expression list.  */
extern int dwarf_getloclist (Dwarf_Attribute *attr, Dwarf_Loc **llbuf,
			     size_t *listlen) __nonnull_attribute__ (2, 3);



/* Return list address ranges.  */
extern int dwarf_getaranges (Dwarf *dbg, Dwarf_Aranges **aranges,
			     size_t *naranges)
     __nonnull_attribute__ (2);

/* Return one of the address range entries.  */
extern Dwarf_Arange *dwarf_onearange (Dwarf_Aranges *aranges, size_t idx);

/* Return information in address range record.  */
extern int dwarf_getarangeinfo (Dwarf_Arange *arange, Dwarf_Addr *addrp,
				Dwarf_Word *lengthp, Dwarf_Off *offsetp);

/* Get address range which includes given address.  */
extern Dwarf_Arange *dwarf_getarange_addr (Dwarf_Aranges *aranges,
					   Dwarf_Addr addr);


/* Call callback function for each of the macro information entry for
   the CU.  */
extern ptrdiff_t dwarf_getmacros (Dwarf_Die *cudie,
				  int (*callback) (Dwarf_Macro *, void *),
				  void *arg, ptrdiff_t offset)
     __nonnull_attribute__ (2);


/* Return error code of last failing function call.  This value is kept
   separately for each thread.  */
extern int dwarf_errno (void);

/* Return error string for ERROR.  If ERROR is zero, return error string
   for most recent error or NULL is none occurred.  If ERROR is -1 the
   behaviour is similar to the last case except that not NULL but a legal
   string is returned.  */
extern const char *dwarf_errmsg (int err);


/* Register new Out-Of-Memory handler.  The old handler is returned.  */
extern Dwarf_OOM dwarf_new_oom_handler (Dwarf *dbg, Dwarf_OOM handler);


/* Inline optimizations.  */
#ifdef __OPTIMIZE__
/* Return attribute code of given attribute.  */
extern inline unsigned int
dwarf_whatattr (Dwarf_Attribute *attr)
{
  return attr == NULL ? 0 : attr->code;
}

/* Return attribute code of given attribute.  */
extern inline unsigned int
dwarf_whatform (Dwarf_Attribute *attr)
{
  return attr == NULL ? 0 : attr->form;
}
#endif	/* Optimize.  */

#endif	/* libdw.h */
