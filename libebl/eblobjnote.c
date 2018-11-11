/* Print contents of object file note.
   Copyright (C) 2002, 2007, 2009, 2011, 2015, 2016, 2018 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libeblP.h>

#include "libelfP.h"


void
ebl_object_note (Ebl *ebl, const char *name, uint32_t type,
		 uint32_t descsz, const char *desc)
{
  if (! ebl->object_note (name, type, descsz, desc))
    {
      /* The machine specific function did not know this type.  */

      if (strcmp ("stapsdt", name) == 0)
	{
	  if (type != 3)
	    {
	      printf (gettext ("unknown SDT version %u\n"), type);
	      return;
	    }

	  /* Descriptor starts with three addresses, pc, base ref and
	     semaphore.  Then three zero terminated strings provider,
	     name and arguments.  */

	  union
	  {
	    Elf64_Addr a64[3];
	    Elf32_Addr a32[3];
	  } addrs;

	  size_t addrs_size = gelf_fsize (ebl->elf, ELF_T_ADDR, 3, EV_CURRENT);
	  if (descsz < addrs_size + 3)
	    {
	    invalid_sdt:
	      printf (gettext ("invalid SDT probe descriptor\n"));
	      return;
	    }

	  Elf_Data src =
	    {
	      .d_type = ELF_T_ADDR, .d_version = EV_CURRENT,
	      .d_buf = (void *) desc, .d_size = addrs_size
	    };

	  Elf_Data dst =
	    {
	      .d_type = ELF_T_ADDR, .d_version = EV_CURRENT,
	      .d_buf = &addrs, .d_size = addrs_size
	    };

	  if (gelf_xlatetom (ebl->elf, &dst, &src,
			     elf_getident (ebl->elf, NULL)[EI_DATA]) == NULL)
	    {
	      printf ("%s\n", elf_errmsg (-1));
	      return;
	    }

	  const char *provider = desc + addrs_size;
	  const char *pname = memchr (provider, '\0', desc + descsz - provider);
	  if (pname == NULL)
	    goto invalid_sdt;

	  ++pname;
	  const char *args = memchr (pname, '\0', desc + descsz - pname);
	  if (args == NULL ||
	      memchr (++args, '\0', desc + descsz - pname) != desc + descsz - 1)
	    goto invalid_sdt;

	  GElf_Addr pc;
	  GElf_Addr base;
	  GElf_Addr sem;
	  if (gelf_getclass (ebl->elf) == ELFCLASS32)
	    {
	      pc = addrs.a32[0];
	      base = addrs.a32[1];
	      sem = addrs.a32[2];
	    }
	  else
	    {
	      pc = addrs.a64[0];
	      base = addrs.a64[1];
	      sem = addrs.a64[2];
	    }

	  printf (gettext ("    PC: "));
	  printf ("%#" PRIx64 ",", pc);
	  printf (gettext (" Base: "));
	  printf ("%#" PRIx64 ",", base);
	  printf (gettext (" Semaphore: "));
	  printf ("%#" PRIx64 "\n", sem);
	  printf (gettext ("    Provider: "));
	  printf ("%s,", provider);
	  printf (gettext (" Name: "));
	  printf ("%s,", pname);
	  printf (gettext (" Args: "));
	  printf ("'%s'\n", args);
	  return;
	}

      /* NT_VERSION doesn't have any info.  All data is in the name.  */
      if (descsz == 0 && type == NT_VERSION)
	return;

      /* Everything else should have the "GNU" owner name.  */
      if (strcmp ("GNU", name) != 0)
	return;

      switch (type)
	{
	case NT_GNU_BUILD_ID:
	  if (strcmp (name, "GNU") == 0 && descsz > 0)
	    {
	      printf (gettext ("    Build ID: "));
	      uint_fast32_t i;
	      for (i = 0; i < descsz - 1; ++i)
		printf ("%02" PRIx8, (uint8_t) desc[i]);
	      printf ("%02" PRIx8 "\n", (uint8_t) desc[i]);
	    }
	  break;

	case NT_GNU_GOLD_VERSION:
	  if (strcmp (name, "GNU") == 0 && descsz > 0)
	    /* A non-null terminated version string.  */
	    printf (gettext ("    Linker version: %.*s\n"),
		    (int) descsz, desc);
	  break;

	case NT_GNU_PROPERTY_TYPE_0:
	  if (strcmp (name, "GNU") == 0 && descsz > 0)
	    {
	      /* There are at least 2 words. type and datasz.  */
	      while (descsz >= 8)
		{
		  struct pr_prop
		  {
		    GElf_Word pr_type;
		    GElf_Word pr_datasz;
		  } prop;

		  Elf_Data in =
		    {
		      .d_version = EV_CURRENT,
		      .d_type = ELF_T_WORD,
		      .d_size = 8,
		      .d_buf = (void *) desc
		    };
		  Elf_Data out =
		    {
		      .d_version = EV_CURRENT,
		      .d_type = ELF_T_WORD,
		      .d_size = descsz,
		      .d_buf = (void *) &prop
		    };

		  if (gelf_xlatetom (ebl->elf, &out, &in,
				     elf_getident (ebl->elf,
						   NULL)[EI_DATA]) == NULL)
		    {
		      printf ("%s\n", elf_errmsg (-1));
		      return;
		    }

		  desc += 8;
		  descsz -= 8;

		  int elfclass = gelf_getclass (ebl->elf);
		  char *elfident = elf_getident (ebl->elf, NULL);
		  GElf_Ehdr ehdr;
		  gelf_getehdr (ebl->elf, &ehdr);

		  /* Prefix.  */
		  printf ("    ");
		  if (prop.pr_type == GNU_PROPERTY_STACK_SIZE)
		    {
		      printf ("STACK_SIZE ");
		      if (prop.pr_datasz == 4 || prop.pr_datasz == 8)
			{
			  GElf_Addr addr;
			  in.d_type = ELF_T_ADDR;
			  out.d_type = ELF_T_ADDR;
			  in.d_size = prop.pr_datasz;
			  out.d_size = sizeof (addr);
			  in.d_buf = (void *) desc;
			  out.d_buf = (void *) &addr;

			  if (gelf_xlatetom (ebl->elf, &out, &in,
					     elfident[EI_DATA]) == NULL)
			    {
			      printf ("%s\n", elf_errmsg (-1));
			      return;
			    }
			  printf ("%#" PRIx64 "\n", addr);
			}
		      else
			printf (" (garbage datasz: %" PRIx32 ")\n",
				prop.pr_datasz);
		    }
		  else if (prop.pr_type == GNU_PROPERTY_NO_COPY_ON_PROTECTED)
		    {
		      printf ("NO_COPY_ON_PROTECTION");
		      if (prop.pr_datasz == 0)
			printf ("\n");
		      else
			printf (" (garbage datasz: %" PRIx32 ")\n",
				prop.pr_datasz);
		    }
		  else if (prop.pr_type >= GNU_PROPERTY_LOPROC
		      && prop.pr_type <= GNU_PROPERTY_HIPROC
		      && (ehdr.e_machine == EM_386
			  || ehdr.e_machine == EM_X86_64))
		    {
		      printf ("X86 ");
		      if (prop.pr_type == GNU_PROPERTY_X86_FEATURE_1_AND)
			{
			  printf ("FEATURE_1_AND: ");

			  if (prop.pr_datasz == 4)
			    {
			      GElf_Word data;
			      in.d_type = ELF_T_WORD;
			      out.d_type = ELF_T_WORD;
			      in.d_size = 4;
			      out.d_size = 4;
			      in.d_buf = (void *) desc;
			      out.d_buf = (void *) &data;

			      if (gelf_xlatetom (ebl->elf, &out, &in,
						 elfident[EI_DATA]) == NULL)
				{
				  printf ("%s\n", elf_errmsg (-1));
				  return;
				}
			      printf ("%08" PRIx32 " ", data);

			      if ((data & GNU_PROPERTY_X86_FEATURE_1_IBT)
				  != 0)
				{
				  printf ("IBT");
				  data &= ~GNU_PROPERTY_X86_FEATURE_1_IBT;
				  if (data != 0)
				    printf (" ");
				}

			      if ((data & GNU_PROPERTY_X86_FEATURE_1_SHSTK)
				  != 0)
				{
				  printf ("SHSTK");
				  data &= ~GNU_PROPERTY_X86_FEATURE_1_SHSTK;
				  if (data != 0)
				    printf (" ");
				}

			      if (data != 0)
				printf ("UNKNOWN");
			    }
			  else
			    printf ("<bad datasz: %" PRId32 ">",
				    prop.pr_datasz);

			  printf ("\n");
			}
		      else
			{
			  printf ("%#" PRIx32, prop.pr_type);
			  if (prop.pr_datasz > 0)
			    {
			      printf (" data: ");
			      size_t i;
			      for (i = 0; i < prop.pr_datasz - 1; i++)
				printf ("%02" PRIx8 " ", (uint8_t) desc[i]);
			      printf ("%02" PRIx8 "\n", (uint8_t) desc[i]);
			    }
			}
		    }
		  else
		    {
		      if (prop.pr_type >= GNU_PROPERTY_LOPROC
			  && prop.pr_type <= GNU_PROPERTY_HIPROC)
			printf ("proc_type %#" PRIx32, prop.pr_type);
		      else if (prop.pr_type >= GNU_PROPERTY_LOUSER
			  && prop.pr_type <= GNU_PROPERTY_HIUSER)
			printf ("app_type %#" PRIx32, prop.pr_type);
		      else
			printf ("unknown_type %#" PRIx32, prop.pr_type);

		      if (prop.pr_datasz > 0)
			{
			  printf (" data: ");
			  size_t i;
			  for (i = 0; i < prop.pr_datasz - 1; i++)
			    printf ("%02" PRIx8 " ", (uint8_t) desc[i]);
			  printf ("%02" PRIx8 "\n", (uint8_t) desc[i]);
			}
		    }
		  if (elfclass == ELFCLASS32)
		    {
		      desc += NOTE_ALIGN4 (prop.pr_datasz);
		      descsz -= NOTE_ALIGN4 (prop.pr_datasz);
		    }
		  else
		    {
		      desc += NOTE_ALIGN8 (prop.pr_datasz);
		      descsz -= NOTE_ALIGN8 (prop.pr_datasz);
		    }
		}
	    }
	  break;

	case NT_GNU_ABI_TAG:
	  if (descsz >= 8 && descsz % 4 == 0)
	    {
	      Elf_Data in =
		{
		  .d_version = EV_CURRENT,
		  .d_type = ELF_T_WORD,
		  .d_size = descsz,
		  .d_buf = (void *) desc
		};
	      /* Normally NT_GNU_ABI_TAG is just 4 words (16 bytes).  If it
		 is much (4*) larger dynamically allocate memory to convert.  */
#define FIXED_TAG_BYTES 16
	      uint32_t sbuf[FIXED_TAG_BYTES];
	      uint32_t *buf;
	      if (unlikely (descsz / 4 > FIXED_TAG_BYTES))
		{
		  buf = malloc (descsz);
		  if (unlikely (buf == NULL))
		    return;
		}
	      else
		buf = sbuf;
	      Elf_Data out =
		{
		  .d_version = EV_CURRENT,
		  .d_type = ELF_T_WORD,
		  .d_size = descsz,
		  .d_buf = buf
		};

	      if (elf32_xlatetom (&out, &in, ebl->data) != NULL)
		{
		  const char *os;
		  switch (buf[0])
		    {
		    case ELF_NOTE_OS_LINUX:
		      os = "Linux";
		      break;

		    case ELF_NOTE_OS_GNU:
		      os = "GNU";
		      break;

		    case ELF_NOTE_OS_SOLARIS2:
		      os = "Solaris";
		      break;

		    case ELF_NOTE_OS_FREEBSD:
		      os = "FreeBSD";
		      break;

		    default:
		      os = "???";
		      break;
		    }

		  printf (gettext ("    OS: %s, ABI: "), os);
		  for (size_t cnt = 1; cnt < descsz / 4; ++cnt)
		    {
		      if (cnt > 1)
			putchar_unlocked ('.');
		      printf ("%" PRIu32, buf[cnt]);
		    }
		  putchar_unlocked ('\n');
		}
	      if (descsz / 4 > FIXED_TAG_BYTES)
		free (buf);
	      break;
	    }
	  FALLTHROUGH;

	default:
	  /* Unknown type.  */
	  break;
	}
    }
}
