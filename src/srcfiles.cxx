/* Print the source files of a given ELF file.
   Copyright (C) 2023 Red Hat, Inc.
   This file is part of elfutils.
   Written by Housam Alamour <alamourh@redhat.com>.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "printversion.h"
#include <dwarf.h>
#include <argp.h>
#include <cstring>
#include <set>
#include <string>
#include <cassert>
#include <config.h>

#include <libdwfl.h>
#include <fcntl.h>
#include <iostream>
#include <libdw.h>

using namespace std;

/* Name and version of program.  */
ARGP_PROGRAM_VERSION_HOOK_DEF = print_version;

/* Bug report address.  */
ARGP_PROGRAM_BUG_ADDRESS_DEF = PACKAGE_BUGREPORT;

/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  { NULL, 0, NULL, OPTION_DOC, N_("Output options:"), 1 },
  { "null", '0', NULL, 0,
    N_ ("Separate items by a null instead of a newline."), 0 },
  { "verbose", 'v', NULL, 0,
    N_ ("Increase verbosity of logging messages."), 0 },
  { "cu-only", 'c', NULL, 0, N_ ("Only list the CU names."), 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Short description of program.  */
static const char doc[] = N_("Lists the source files of a DWARF/ELF file. The default input is the file 'a.out'.");

/* Strings for arguments in help texts.  */
static const char args_doc[] = N_("INPUT");

/* Prototype for option handler.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

static struct argp_child argp_children[2]; /* [0] is set in main.  */

/* Data structure to communicate with argp functions.  */
static const struct argp argp =
{
  options, parse_opt, args_doc, doc, argp_children, NULL, NULL
};

/* Verbose message printing. */
static bool verbose;
/* Delimit the output with nulls. */
static bool null_arg;
/* Only print compilation unit names. */
static bool CU_only;

/* Handle program arguments. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Suppress "unused parameter" warning. */
  (void)arg;
  switch (key)
    {
    case ARGP_KEY_INIT:
      state->child_inputs[0] = state->input;
      break;

    case '0':
      null_arg = true;
      break;

    case 'v':
      verbose = true;
      break;

    case 'c':
      CU_only = true;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


/* Global list of collected source files.  Normally, it'll contain
   the sources of just one named binary, but the '-K' option can cause
   multiple dwfl modules to be loaded, thus listed.   */
   set<string> debug_sourcefiles;

static int
collect_sourcefiles (Dwfl_Module *dwflmod,
                     void **userdata __attribute__ ((unused)),
                     const char *name __attribute__ ((unused)),
                     Dwarf_Addr base __attribute__ ((unused)),
                     void *arg __attribute__ ((unused)))
{
  Dwarf *dbg;
  Dwarf_Addr bias; /* ignored - for addressing purposes only  */

  dbg = dwfl_module_getdwarf (dwflmod, &bias);

  Dwarf_Off offset = 0;
  Dwarf_Off old_offset;
  size_t hsize;

  /* Traverse all CUs of this module.  */
  while (dwarf_nextcu (dbg, old_offset = offset, &offset, &hsize, NULL, NULL, NULL) == 0)
    {
      Dwarf_Die cudie_mem;
      Dwarf_Die *cudie = dwarf_offdie (dbg, old_offset + hsize, &cudie_mem);

      if (cudie == NULL)
        continue;

      const char *cuname = dwarf_diename (cudie) ?: "<unknown>";
      Dwarf_Files *files;
      size_t nfiles;
      if (dwarf_getsrcfiles (cudie, &files, &nfiles) != 0)
        continue;

      /* extract DW_AT_comp_dir to resolve relative file names  */
      const char *comp_dir = "";
      const char *const *dirs;
      size_t ndirs;

      if (dwarf_getsrcdirs (files, &dirs, &ndirs) == 0 && dirs[0] != NULL)
        comp_dir = dirs[0];
      if (comp_dir == NULL)
        comp_dir = "";

      if (verbose)
        std::clog << "searching for sources for cu=" << cuname
                  << " comp_dir=" << comp_dir << " #files=" << nfiles
                  << " #dirs=" << ndirs << endl;

      for (size_t f = 1; f < nfiles; f++)
        {
          const char *hat;
          if (CU_only)
          {
            if (strcmp(cuname, "<unknown>") == 0 || strcmp(cuname, "<artificial>") == 0 )
              continue;
            hat = cuname;
          }
          else
            hat = dwarf_filesrc (files, f, NULL, NULL);

          if (hat == NULL)
            continue;

          if (string(hat).find("<built-in>")
              != std::string::npos) /* gcc intrinsics, don't bother record  */
            continue;

          string waldo;
          if (hat[0] == '/') /* absolute */
            waldo = (string (hat));
          else if (comp_dir[0] != '\0') /* comp_dir relative */
            waldo = (string (comp_dir) + string ("/") + string (hat));
          debug_sourcefiles.insert (waldo);
        }
    }

  return DWARF_CB_OK;
}


int
main (int argc, char *argv[])
{
  int remaining;

  /* Parse and process arguments.  This includes opening the modules.  */
  argp_children[0].argp = dwfl_standard_argp ();
  argp_children[0].group = 1;

  Dwfl *dwfl = NULL;
  (void) argp_parse (&argp, argc, argv, 0, &remaining, &dwfl);
  assert (dwfl != NULL);
  /* Process all loaded modules - probably just one, except if -K or -p is used. */
  (void) dwfl_getmodules (dwfl, &collect_sourcefiles, NULL, 0);

  if (!debug_sourcefiles.empty ())
    for (const string &element : debug_sourcefiles)
      {
        std::cout << element;
        if (null_arg)
          std::cout << '\0';
        else
          std::cout << '\n';
      }

  dwfl_end (dwfl);
  return 0;
}
