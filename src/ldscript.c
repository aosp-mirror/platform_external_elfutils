/* A Bison parser, made by GNU Bison 1.875c.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0

/* If NAME_PREFIX is specified substitute the variables and functions
   names.  */
#define yyparse ldparse
#define yylex   ldlex
#define yyerror lderror
#define yylval  ldlval
#define yychar  ldchar
#define yydebug lddebug
#define yynerrs ldnerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     kADD_OP = 258,
     kALIGN = 259,
     kENTRY = 260,
     kEXCLUDE_FILE = 261,
     kFILENAME = 262,
     kGLOBAL = 263,
     kGROUP = 264,
     kID = 265,
     kINPUT = 266,
     kINTERP = 267,
     kKEEP = 268,
     kLOCAL = 269,
     kMODE = 270,
     kMUL_OP = 271,
     kNUM = 272,
     kOUTPUT_FORMAT = 273,
     kPAGESIZE = 274,
     kPROVIDE = 275,
     kSEARCH_DIR = 276,
     kSEGMENT = 277,
     kSIZEOF_HEADERS = 278,
     kSORT = 279,
     kVERSION = 280,
     kVERSION_SCRIPT = 281,
     ADD_OP = 282,
     MUL_OP = 283
   };
#endif
#define kADD_OP 258
#define kALIGN 259
#define kENTRY 260
#define kEXCLUDE_FILE 261
#define kFILENAME 262
#define kGLOBAL 263
#define kGROUP 264
#define kID 265
#define kINPUT 266
#define kINTERP 267
#define kKEEP 268
#define kLOCAL 269
#define kMODE 270
#define kMUL_OP 271
#define kNUM 272
#define kOUTPUT_FORMAT 273
#define kPAGESIZE 274
#define kPROVIDE 275
#define kSEARCH_DIR 276
#define kSEGMENT 277
#define kSIZEOF_HEADERS 278
#define kSORT 279
#define kVERSION 280
#define kVERSION_SCRIPT 281
#define ADD_OP 282
#define MUL_OP 283




/* Copy the first part of user declarations.  */
#line 1 "/home/drepper/gnu/elfutils/src/ldscript.y"

/* Parser for linker scripts.
   Copyright (C) 2001, 2002, 2003, 2004 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2001.

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

#include <assert.h>
#include <error.h>
#include <libintl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <system.h>
#include <ld.h>

/* The error handler.  */
static void yyerror (const char *s);

/* Some helper functions we need to construct the data structures
   describing information from the file.  */
static struct expression *new_expr (int tag);
static struct input_section_name *new_input_section_name (const char *name,
							  bool sort_flag);
static struct input_rule *new_input_rule (int tag);
static struct output_rule *new_output_rule (int tag);
static struct assignment *new_assignment (const char *variable,
					  struct expression *expression,
					  bool provide_flag);
static void new_segment (int mode, struct output_rule *output_rule);
static struct filename_list *new_filename_listelem (const char *string);
static void add_inputfiles (struct filename_list *fnames);
static struct id_list *new_id_listelem (const char *str);
static struct version *new_version (struct id_list *local,
				    struct id_list *global);
static struct version *merge_versions (struct version *one,
				       struct version *two);
static void add_versions (struct version *versions);

extern int yylex (void);


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 58 "/home/drepper/gnu/elfutils/src/ldscript.y"
typedef union YYSTYPE {
  uintmax_t num;
  enum expression_tag op;
  char *str;
  struct expression *expr;
  struct input_section_name *sectionname;
  struct filemask_section_name *filemask_section_name;
  struct input_rule *input_rule;
  struct output_rule *output_rule;
  struct assignment *assignment;
  struct filename_list *filename_list;
  struct version *version;
  struct id_list *id_list;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 213 "ldscript.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 225 "ldscript.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  30
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   198

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  39
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  22
/* YYNRULES -- Number of rules. */
#define YYNRULES  62
/* YYNRULES -- Number of states. */
#define YYNSTATES  146

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   283

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    28,     2,
      32,    33,    30,     2,    38,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    34,
       2,    37,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    35,    27,    36,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    29,    31
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     8,    11,    13,    19,    25,    31,
      37,    43,    49,    54,    59,    64,    69,    72,    74,    77,
      82,    85,    89,    96,    99,   101,   103,   108,   111,   117,
     119,   124,   129,   130,   135,   139,   143,   147,   151,   155,
     159,   161,   163,   165,   167,   171,   173,   175,   176,   179,
     181,   186,   192,   199,   202,   204,   207,   210,   214,   217,
     219,   221,   223
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      40,     0,    -1,    41,    -1,    26,    54,    -1,    41,    42,
      -1,    42,    -1,     5,    32,    10,    33,    34,    -1,    21,
      32,    59,    33,    34,    -1,    19,    32,    17,    33,    34,
      -1,    12,    32,    59,    33,    34,    -1,    22,    15,    35,
      43,    36,    -1,    22,     1,    35,    43,    36,    -1,     9,
      32,    52,    33,    -1,    11,    32,    52,    33,    -1,    25,
      35,    54,    36,    -1,    18,    32,    59,    33,    -1,    43,
      44,    -1,    44,    -1,    45,    34,    -1,    10,    35,    46,
      36,    -1,    10,    34,    -1,    10,    37,    51,    -1,    20,
      32,    10,    37,    51,    33,    -1,    46,    47,    -1,    47,
      -1,    48,    -1,    13,    32,    48,    33,    -1,    45,    34,
      -1,    60,    32,    50,    49,    33,    -1,    10,    -1,    24,
      32,    10,    33,    -1,     6,    32,    59,    33,    -1,    -1,
       4,    32,    51,    33,    -1,    32,    51,    33,    -1,    51,
      30,    51,    -1,    51,    16,    51,    -1,    51,     3,    51,
      -1,    51,    28,    51,    -1,    51,    27,    51,    -1,    17,
      -1,    10,    -1,    23,    -1,    19,    -1,    52,    53,    59,
      -1,    59,    -1,    38,    -1,    -1,    54,    55,    -1,    55,
      -1,    35,    56,    36,    34,    -1,    59,    35,    56,    36,
      34,    -1,    59,    35,    56,    36,    59,    34,    -1,    56,
      57,    -1,    57,    -1,     8,    58,    -1,    14,    58,    -1,
      58,    60,    34,    -1,    60,    34,    -1,     7,    -1,    10,
      -1,    59,    -1,    30,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   128,   128,   129,   133,   134,   137,   142,   146,   151,
     156,   160,   166,   177,   179,   181,   185,   190,   194,   199,
     211,   235,   237,   241,   246,   250,   255,   262,   269,   280,
     282,   286,   289,   292,   297,   299,   305,   311,   317,   323,
     329,   334,   339,   341,   345,   351,   355,   356,   359,   364,
     368,   374,   380,   389,   391,   395,   397,   402,   408,   412,
     414,   418,   420
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "kADD_OP", "kALIGN", "kENTRY",
  "kEXCLUDE_FILE", "kFILENAME", "kGLOBAL", "kGROUP", "kID", "kINPUT",
  "kINTERP", "kKEEP", "kLOCAL", "kMODE", "kMUL_OP", "kNUM",
  "kOUTPUT_FORMAT", "kPAGESIZE", "kPROVIDE", "kSEARCH_DIR", "kSEGMENT",
  "kSIZEOF_HEADERS", "kSORT", "kVERSION", "kVERSION_SCRIPT", "'|'", "'&'",
  "ADD_OP", "'*'", "MUL_OP", "'('", "')'", "';'", "'{'", "'}'", "'='",
  "','", "$accept", "script_or_version", "file", "content",
  "outputsections", "outputsection", "assignment", "inputsections",
  "inputsection", "sectionname", "sort_opt_name", "exclude_opt", "expr",
  "filename_id_list", "comma_opt", "versionlist", "version",
  "version_stmt_list", "version_stmt", "filename_id_star_list",
  "filename_id", "filename_id_star", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   124,    38,   282,
      42,   283,    40,    41,    59,   123,   125,    61,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    39,    40,    40,    41,    41,    42,    42,    42,    42,
      42,    42,    42,    42,    42,    42,    43,    43,    44,    44,
      44,    45,    45,    46,    46,    47,    47,    47,    48,    49,
      49,    50,    50,    51,    51,    51,    51,    51,    51,    51,
      51,    51,    51,    51,    52,    52,    53,    53,    54,    54,
      55,    55,    55,    56,    56,    57,    57,    58,    58,    59,
      59,    60,    60
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     2,     2,     1,     5,     5,     5,     5,
       5,     5,     4,     4,     4,     4,     2,     1,     2,     4,
       2,     3,     6,     2,     1,     1,     4,     2,     5,     1,
       4,     4,     0,     4,     3,     3,     3,     3,     3,     3,
       1,     1,     1,     1,     3,     1,     1,     0,     2,     1,
       4,     5,     6,     2,     1,     2,     2,     3,     2,     1,
       1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     2,     5,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    59,    60,     0,     3,    49,     0,
       1,     4,     0,    47,    45,    47,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    54,    48,     0,     0,
      12,    46,     0,    13,     0,    15,     0,     0,     0,     0,
       0,    17,     0,     0,    14,    62,    55,    61,     0,    56,
       0,    53,     0,     6,    44,     9,     8,     7,    20,     0,
       0,     0,    11,    16,    18,    10,     0,    58,    50,     0,
      60,     0,     0,     0,    24,    25,     0,     0,    41,    40,
      43,    42,     0,    21,     0,    57,    51,     0,     0,    27,
      19,    23,    32,     0,     0,     0,     0,     0,     0,     0,
       0,    52,     0,     0,     0,     0,    34,    37,    36,    39,
      38,    35,     0,    26,     0,    29,     0,     0,    33,    22,
       0,     0,    28,    31,     0,    30
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,    11,    12,    13,    60,    61,    62,    93,    94,    95,
     137,   124,   103,    33,    52,    27,    28,    45,    46,    66,
      67,    96
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -41
static const short yypact[] =
{
     107,   -28,   -20,   -13,    34,    77,    85,    88,    91,    33,
      38,   123,   125,   -41,   117,    52,    52,    52,    52,   114,
      52,   100,   103,    38,   -41,   -41,    96,    38,   -41,   110,
     -41,   -41,   115,    64,   -41,    67,   116,   118,   120,   127,
       1,     1,    28,    84,    84,    36,   -41,   -41,    96,   128,
     -41,   -41,    52,   -41,   129,   -41,   130,   131,   105,   134,
      75,   -41,   133,    79,   -41,   -41,    84,   -41,   135,    84,
     136,   -41,    41,   -41,   -41,   -41,   -41,   -41,   -41,    83,
      48,   151,   -41,   -41,   -41,   -41,   137,   -41,   -41,    44,
     138,   140,   139,    17,   -41,   -41,   142,   144,   -41,   -41,
     -41,   -41,    48,    54,   141,   -41,   -41,   143,    84,   -41,
     -41,   -41,   162,    48,    -2,    48,    48,    48,    48,    48,
      48,   -41,   146,   148,    97,     6,   -41,    54,    54,    58,
      53,    -1,    13,   -41,    52,   -41,   149,   150,   -41,   -41,
     152,   172,   -41,   -41,   153,   -41
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
     -41,   -41,   -41,   175,   147,   -40,    29,   -41,    98,    76,
     -41,   -41,    39,   173,   -41,   167,   -24,   145,    15,   154,
     -10,    32
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      29,   115,   115,    47,    14,    34,    34,    36,    37,   115,
      39,    58,    15,    29,   116,   116,   115,    29,    47,    16,
      83,    59,   116,    83,    24,   117,   118,    90,   119,   116,
      91,   126,    29,   117,   118,    24,   119,    59,    25,   138,
     117,   118,    74,   119,    43,    24,   139,    65,    25,    43,
      44,    24,    97,   110,    25,    44,   115,   115,    98,    24,
      71,   115,    25,    26,    64,    99,    17,   100,    23,   116,
     116,   101,    70,    26,   116,    68,    68,    89,   106,   107,
     102,   117,   118,   119,   119,    58,   118,    71,   119,    58,
      24,    24,    21,    90,    25,    59,    91,    50,    86,    59,
      53,    86,    51,    59,    43,    51,    22,   135,    92,    18,
      44,    82,     1,    65,    65,    85,     2,    19,     3,     4,
      20,   136,    92,    30,   140,     5,     6,    32,     7,     8,
       1,    38,     9,    10,     2,    40,     3,     4,    41,    78,
      79,   114,    80,     5,     6,    48,     7,     8,    49,    54,
       9,    55,   125,    56,   127,   128,   129,   130,   131,   132,
      57,   104,    73,    75,    76,    77,    81,    84,   123,    87,
      88,   105,   108,   109,   112,    80,   113,   121,   120,   133,
     134,   141,   144,   142,   122,   143,   145,    31,    63,    35,
      42,   111,     0,    72,     0,     0,     0,     0,    69
};

static const short yycheck[] =
{
      10,     3,     3,    27,    32,    15,    16,    17,    18,     3,
      20,    10,    32,    23,    16,    16,     3,    27,    42,    32,
      60,    20,    16,    63,     7,    27,    28,    10,    30,    16,
      13,    33,    42,    27,    28,     7,    30,    20,    10,    33,
      27,    28,    52,    30,     8,     7,    33,    30,    10,     8,
      14,     7,     4,    36,    10,    14,     3,     3,    10,     7,
      45,     3,    10,    35,    36,    17,    32,    19,    35,    16,
      16,    23,    36,    35,    16,    43,    44,    36,    34,    89,
      32,    27,    28,    30,    30,    10,    28,    72,    30,    10,
       7,     7,     1,    10,    10,    20,    13,    33,    66,    20,
      33,    69,    38,    20,     8,    38,    15,    10,    79,    32,
      14,    36,     5,    30,    30,    36,     9,    32,    11,    12,
      32,    24,    93,     0,   134,    18,    19,    10,    21,    22,
       5,    17,    25,    26,     9,    35,    11,    12,    35,    34,
      35,   102,    37,    18,    19,    35,    21,    22,    33,    33,
      25,    33,   113,    33,   115,   116,   117,   118,   119,   120,
      33,    10,    34,    34,    34,    34,    32,    34,     6,    34,
      34,    34,    32,    34,    32,    37,    32,    34,    37,    33,
      32,    32,    10,    33,   108,    33,    33,    12,    41,    16,
      23,    93,    -1,    48,    -1,    -1,    -1,    -1,    44
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     5,     9,    11,    12,    18,    19,    21,    22,    25,
      26,    40,    41,    42,    32,    32,    32,    32,    32,    32,
      32,     1,    15,    35,     7,    10,    35,    54,    55,    59,
       0,    42,    10,    52,    59,    52,    59,    59,    17,    59,
      35,    35,    54,     8,    14,    56,    57,    55,    35,    33,
      33,    38,    53,    33,    33,    33,    33,    33,    10,    20,
      43,    44,    45,    43,    36,    30,    58,    59,    60,    58,
      36,    57,    56,    34,    59,    34,    34,    34,    34,    35,
      37,    32,    36,    44,    34,    36,    60,    34,    34,    36,
      10,    13,    45,    46,    47,    48,    60,     4,    10,    17,
      19,    23,    32,    51,    10,    34,    34,    59,    32,    34,
      36,    47,    32,    32,    51,     3,    16,    27,    28,    30,
      37,    34,    48,     6,    50,    51,    33,    51,    51,    51,
      51,    51,    51,    33,    32,    10,    24,    49,    33,    33,
      59,    32,    33,    33,    10,    33
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 130 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { add_versions (yyvsp[0].version); }
    break;

  case 6:
#line 138 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      if (likely (ld_state.entry == NULL))
			ld_state.entry = yyvsp[-2].str;
		    }
    break;

  case 7:
#line 143 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      ld_new_searchdir (yyvsp[-2].str);
		    }
    break;

  case 8:
#line 147 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      if (likely (ld_state.pagesize == 0))
			ld_state.pagesize = yyvsp[-2].num;
		    }
    break;

  case 9:
#line 152 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      if (likely (ld_state.interp == NULL))
			ld_state.interp = yyvsp[-2].str;
		    }
    break;

  case 10:
#line 157 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      new_segment (yyvsp[-3].num, yyvsp[-1].output_rule);
		    }
    break;

  case 11:
#line 161 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      fputs_unlocked (gettext ("mode for segment invalid\n"),
				      stderr);
		      new_segment (0, yyvsp[-1].output_rule);
		    }
    break;

  case 12:
#line 167 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      /* First little optimization.  If there is only one
			 file in the group don't do anything.  */
		      if (yyvsp[-1].filename_list != yyvsp[-1].filename_list->next)
			{
			  yyvsp[-1].filename_list->next->group_start = 1;
			  yyvsp[-1].filename_list->group_end = 1;
			}
		      add_inputfiles (yyvsp[-1].filename_list);
		    }
    break;

  case 13:
#line 178 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { add_inputfiles (yyvsp[-1].filename_list); }
    break;

  case 14:
#line 180 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { add_versions (yyvsp[-1].version); }
    break;

  case 15:
#line 182 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { /* XXX TODO */ }
    break;

  case 16:
#line 186 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyvsp[0].output_rule->next = yyvsp[-1].output_rule->next;
		      yyval.output_rule = yyvsp[-1].output_rule->next = yyvsp[0].output_rule;
		    }
    break;

  case 17:
#line 191 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.output_rule = yyvsp[0].output_rule; }
    break;

  case 18:
#line 195 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.output_rule = new_output_rule (output_assignment);
		      yyval.output_rule->val.assignment = yyvsp[-1].assignment;
		    }
    break;

  case 19:
#line 200 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.output_rule = new_output_rule (output_section);
		      yyval.output_rule->val.section.name = yyvsp[-3].str;
		      yyval.output_rule->val.section.input = yyvsp[-1].input_rule->next;
		      if (ld_state.strip == strip_debug
			  && ebl_debugscn_p (ld_state.ebl, yyvsp[-3].str))
			yyval.output_rule->val.section.ignored = true;
		      else
			yyval.output_rule->val.section.ignored = false;
		      yyvsp[-1].input_rule->next = NULL;
		    }
    break;

  case 20:
#line 212 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      /* This is a short cut for "ID { *(ID) }".  */
		      yyval.output_rule = new_output_rule (output_section);
		      yyval.output_rule->val.section.name = yyvsp[-1].str;
		      yyval.output_rule->val.section.input = new_input_rule (input_section);
		      yyval.output_rule->val.section.input->next = NULL;
		      yyval.output_rule->val.section.input->val.section =
			(struct filemask_section_name *)
			  obstack_alloc (&ld_state.smem,
					 sizeof (struct filemask_section_name));
		      yyval.output_rule->val.section.input->val.section->filemask = NULL;
		      yyval.output_rule->val.section.input->val.section->excludemask = NULL;
		      yyval.output_rule->val.section.input->val.section->section_name =
			new_input_section_name (yyvsp[-1].str, false);
		      yyval.output_rule->val.section.input->val.section->keep_flag = false;
		      if (ld_state.strip == strip_debug
			  && ebl_debugscn_p (ld_state.ebl, yyvsp[-1].str))
			yyval.output_rule->val.section.ignored = true;
		      else
			yyval.output_rule->val.section.ignored = false;
		    }
    break;

  case 21:
#line 236 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.assignment = new_assignment (yyvsp[-2].str, yyvsp[0].expr, false); }
    break;

  case 22:
#line 238 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.assignment = new_assignment (yyvsp[-3].str, yyvsp[-1].expr, true); }
    break;

  case 23:
#line 242 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyvsp[0].input_rule->next = yyvsp[-1].input_rule->next;
		      yyval.input_rule = yyvsp[-1].input_rule->next = yyvsp[0].input_rule;
		    }
    break;

  case 24:
#line 247 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.input_rule = yyvsp[0].input_rule; }
    break;

  case 25:
#line 251 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.input_rule = new_input_rule (input_section);
		      yyval.input_rule->val.section = yyvsp[0].filemask_section_name;
		    }
    break;

  case 26:
#line 256 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyvsp[-1].filemask_section_name->keep_flag = true;

		      yyval.input_rule = new_input_rule (input_section);
		      yyval.input_rule->val.section = yyvsp[-1].filemask_section_name;
		    }
    break;

  case 27:
#line 263 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.input_rule = new_input_rule (input_assignment);
		      yyval.input_rule->val.assignment = yyvsp[-1].assignment;
		    }
    break;

  case 28:
#line 270 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.filemask_section_name = (struct filemask_section_name *)
			obstack_alloc (&ld_state.smem, sizeof (*yyval.filemask_section_name));
		      yyval.filemask_section_name->filemask = yyvsp[-4].str;
		      yyval.filemask_section_name->excludemask = yyvsp[-2].str;
		      yyval.filemask_section_name->section_name = yyvsp[-1].sectionname;
		      yyval.filemask_section_name->keep_flag = false;
		    }
    break;

  case 29:
#line 281 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.sectionname = new_input_section_name (yyvsp[0].str, false); }
    break;

  case 30:
#line 283 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.sectionname = new_input_section_name (yyvsp[-1].str, true); }
    break;

  case 31:
#line 287 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.str = yyvsp[-1].str; }
    break;

  case 32:
#line 289 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.str = NULL; }
    break;

  case 33:
#line 293 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.expr = new_expr (exp_align);
		      yyval.expr->val.child = yyvsp[-1].expr;
		    }
    break;

  case 34:
#line 298 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.expr = yyvsp[-1].expr; }
    break;

  case 35:
#line 300 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.expr = new_expr (exp_mult);
		      yyval.expr->val.binary.left = yyvsp[-2].expr;
		      yyval.expr->val.binary.right = yyvsp[0].expr;
		    }
    break;

  case 36:
#line 306 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.expr = new_expr (yyvsp[-1].op);
		      yyval.expr->val.binary.left = yyvsp[-2].expr;
		      yyval.expr->val.binary.right = yyvsp[0].expr;
		    }
    break;

  case 37:
#line 312 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.expr = new_expr (yyvsp[-1].op);
		      yyval.expr->val.binary.left = yyvsp[-2].expr;
		      yyval.expr->val.binary.right = yyvsp[0].expr;
		    }
    break;

  case 38:
#line 318 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.expr = new_expr (exp_and);
		      yyval.expr->val.binary.left = yyvsp[-2].expr;
		      yyval.expr->val.binary.right = yyvsp[0].expr;
		    }
    break;

  case 39:
#line 324 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.expr = new_expr (exp_or);
		      yyval.expr->val.binary.left = yyvsp[-2].expr;
		      yyval.expr->val.binary.right = yyvsp[0].expr;
		    }
    break;

  case 40:
#line 330 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.expr = new_expr (exp_num);
		      yyval.expr->val.num = yyvsp[0].num;
		    }
    break;

  case 41:
#line 335 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyval.expr = new_expr (exp_id);
		      yyval.expr->val.str = yyvsp[0].str;
		    }
    break;

  case 42:
#line 340 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.expr = new_expr (exp_sizeof_headers); }
    break;

  case 43:
#line 342 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.expr = new_expr (exp_pagesize); }
    break;

  case 44:
#line 346 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      struct filename_list *newp = new_filename_listelem (yyvsp[0].str);
		      newp->next = yyvsp[-2].filename_list->next;
		      yyval.filename_list = yyvsp[-2].filename_list->next = newp;
		    }
    break;

  case 45:
#line 352 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.filename_list = new_filename_listelem (yyvsp[0].str); }
    break;

  case 48:
#line 360 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyvsp[0].version->next = yyvsp[-1].version->next;
		      yyval.version = yyvsp[-1].version->next = yyvsp[0].version;
		    }
    break;

  case 49:
#line 365 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.version = yyvsp[0].version; }
    break;

  case 50:
#line 369 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyvsp[-2].version->versionname = "";
		      yyvsp[-2].version->parentname = NULL;
		      yyval.version = yyvsp[-2].version;
		    }
    break;

  case 51:
#line 375 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyvsp[-2].version->versionname = yyvsp[-4].str;
		      yyvsp[-2].version->parentname = NULL;
		      yyval.version = yyvsp[-2].version;
		    }
    break;

  case 52:
#line 381 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      yyvsp[-3].version->versionname = yyvsp[-5].str;
		      yyvsp[-3].version->parentname = yyvsp[-1].str;
		      yyval.version = yyvsp[-3].version;
		    }
    break;

  case 53:
#line 390 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.version = merge_versions (yyvsp[-1].version, yyvsp[0].version); }
    break;

  case 54:
#line 392 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.version = yyvsp[0].version; }
    break;

  case 55:
#line 396 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.version = new_version (NULL, yyvsp[0].id_list); }
    break;

  case 56:
#line 398 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.version = new_version (yyvsp[0].id_list, NULL); }
    break;

  case 57:
#line 403 "/home/drepper/gnu/elfutils/src/ldscript.y"
    {
		      struct id_list *newp = new_id_listelem (yyvsp[-1].str);
		      newp->next = yyvsp[-2].id_list->next;
		      yyval.id_list = yyvsp[-2].id_list->next = newp;
		    }
    break;

  case 58:
#line 409 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.id_list = new_id_listelem (yyvsp[-1].str); }
    break;

  case 59:
#line 413 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 60:
#line 415 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 61:
#line 419 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 62:
#line 421 "/home/drepper/gnu/elfutils/src/ldscript.y"
    { yyval.str = NULL; }
    break;


    }

/* Line 1000 of yacc.c.  */
#line 1654 "ldscript.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
	  yychar = YYEMPTY;

	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 424 "/home/drepper/gnu/elfutils/src/ldscript.y"


static void
yyerror (const char *s)
{
  error (0, 0, (ld_scan_version_script
		? gettext ("while reading version script '%s': %s at line %d")
		: gettext ("while reading linker script '%s': %s at line %d")),
	 ldin_fname, gettext (s), ldlineno);
}


static struct expression *
new_expr (int tag)
{
  struct expression *newp = (struct expression *)
    obstack_alloc (&ld_state.smem, sizeof (*newp));

  newp->tag = tag;
  return newp;
}


static struct input_section_name *
new_input_section_name (const char *name, bool sort_flag)
{
  struct input_section_name *newp = (struct input_section_name *)
    obstack_alloc (&ld_state.smem, sizeof (*newp));

  newp->name = name;
  newp->sort_flag = sort_flag;
  return newp;
}


static struct input_rule *
new_input_rule (int tag)
{
  struct input_rule *newp = (struct input_rule *)
    obstack_alloc (&ld_state.smem, sizeof (*newp));

  newp->tag = tag;
  newp->next = newp;
  return newp;
}


static struct output_rule *
new_output_rule (int tag)
{
  struct output_rule *newp = (struct output_rule *)
    memset (obstack_alloc (&ld_state.smem, sizeof (*newp)),
	    '\0', sizeof (*newp));

  newp->tag = tag;
  newp->next = newp;
  return newp;
}


static struct assignment *
new_assignment (const char *variable, struct expression *expression,
		bool provide_flag)
{
  struct assignment *newp = (struct assignment *)
    obstack_alloc (&ld_state.smem, sizeof (*newp));

  newp->variable = variable;
  newp->expression = expression;
  newp->sym = NULL;
  newp->provide_flag = provide_flag;

  /* Insert the symbol into a hash table.  We will later have to matc*/
  return newp;
}


static void
new_segment (int mode, struct output_rule *output_rule)
{
  struct output_segment *newp;

  newp
    = (struct output_segment *) obstack_alloc (&ld_state.smem, sizeof (*newp));
  newp->mode = mode;
  newp->next = newp;

  newp->output_rules = output_rule->next;
  output_rule->next = NULL;

  /* Enqueue the output segment description.  */
  if (ld_state.output_segments == NULL)
    ld_state.output_segments = newp;
  else
    {
      newp->next = ld_state.output_segments->next;
      ld_state.output_segments = ld_state.output_segments->next = newp;
    }

  /* If the output file should be stripped of all symbol set the flag
     in the structures of all output sections.  */
  if (mode == 0 && ld_state.strip == strip_all)
    {
      struct output_rule *runp;

      for (runp = newp->output_rules; runp != NULL; runp = runp->next)
	if (runp->tag == output_section)
	  runp->val.section.ignored = true;
    }
}


static struct filename_list *
new_filename_listelem (const char *string)
{
  struct filename_list *newp;

  /* We use calloc and not the obstack since this object can be freed soon.  */
  newp = (struct filename_list *) xcalloc (1, sizeof (*newp));
  newp->name = string;
  newp->next = newp;
  return newp;
}


static void
add_inputfiles (struct filename_list *fnames)
{
  assert (fnames != NULL);

  if (ld_state.srcfiles == NULL)
    ld_state.srcfiles = fnames;
  else
    {
      struct filename_list *first = ld_state.srcfiles->next;

      ld_state.srcfiles->next = fnames->next;
      fnames->next = first;
      ld_state.srcfiles->next = fnames;
    }
}


static _Bool
special_char_p (const char *str)
{
  while (*str != '\0')
    {
      if (__builtin_expect (*str == '*', 0)
	  || __builtin_expect (*str == '?', 0)
	  || __builtin_expect (*str == '[', 0))
	return true;

      ++str;
    }

  return false;
}


static struct id_list *
new_id_listelem (const char *str)
{
  struct id_list *newp;

  newp = (struct id_list *) obstack_alloc (&ld_state.smem, sizeof (*newp));
  if (str == NULL)
    newp->u.id_type = id_all;
  else if (__builtin_expect (special_char_p (str), false))
    newp->u.id_type = id_wild;
  else
    newp->u.id_type = id_str;
  newp->id = str;
  newp->next = newp;

  return newp;
}


static struct version *
new_version (struct id_list *local, struct id_list *global)
{
  struct version *newp;

  newp = (struct version *) obstack_alloc (&ld_state.smem, sizeof (*newp));
  newp->next = newp;
  newp->local_names = local;
  newp->global_names = global;
  newp->versionname = NULL;
  newp->parentname = NULL;

  return newp;
}


static struct version *
merge_versions (struct version *one, struct version *two)
{
  assert (two->local_names == NULL || two->global_names == NULL);

  if (two->local_names != NULL)
    {
      if (one->local_names == NULL)
	one->local_names = two->local_names;
      else
	{
	  two->local_names->next = one->local_names->next;
	  one->local_names = one->local_names->next = two->local_names;
	}
    }
  else
    {
      if (one->global_names == NULL)
	one->global_names = two->global_names;
      else
	{
	  two->global_names->next = one->global_names->next;
	  one->global_names = one->global_names->next = two->global_names;
	}
    }

  return one;
}


static void
add_id_list (const char *versionname, struct id_list *runp, _Bool local)
{
  struct id_list *lastp = runp;

  if (runp == NULL)
    /* Nothing to do.  */
    return;

  /* Convert into a simple single-linked list.  */
  runp = runp->next;
  assert (runp != NULL);
  lastp->next = NULL;

  do
    if (runp->u.id_type == id_str)
      {
	struct id_list *curp;
	struct id_list *defp;
	unsigned long int hval = elf_hash (runp->id);

	curp = runp;
	runp = runp->next;

	defp = ld_version_str_tab_find (&ld_state.version_str_tab, hval, curp);
	if (defp != NULL)
	  {
	    /* There is already a version definition for this symbol.  */
	    while (strcmp (defp->u.s.versionname, versionname) != 0)
	      {
		if (defp->next == NULL)
		  {
		    /* No version like this so far.  */
		    defp->next = curp;
		    curp->u.s.local = local;
		    curp->u.s.versionname = versionname;
		    curp->next = NULL;
		    defp = NULL;
		    break;
		  }

		defp = defp->next;
	      }

	    if (defp != NULL && defp->u.s.local != local)
	      error (EXIT_FAILURE, 0, versionname[0] == '\0'
		     ? gettext ("\
symbol '%s' in declared both local and global for unnamed version")
		     : gettext ("\
symbol '%s' in declared both local and global for version '%s'"),
		     runp->id, versionname);
	  }
	else
	  {
	    /* This is the first version definition for this symbol.  */
	    ld_version_str_tab_insert (&ld_state.version_str_tab, hval, curp);

	    curp->u.s.local = local;
	    curp->u.s.versionname = versionname;
	    curp->next = NULL;
	  }
      }
    else if (runp->u.id_type == id_all)
      {
	if (local)
	  {
	    if (ld_state.default_bind_global)
	      error (EXIT_FAILURE, 0,
		     gettext ("default visibility set as local and global"));
	    ld_state.default_bind_local = true;
	  }
	else
	  {
	    if (ld_state.default_bind_local)
	      error (EXIT_FAILURE, 0,
		     gettext ("default visibility set as local and global"));
	    ld_state.default_bind_global = true;
	  }

	runp = runp->next;
      }
    else
      {
	assert (runp->u.id_type == id_wild);
	/* XXX TBI */
	abort ();
      }
  while (runp != NULL);
}


static void
add_versions (struct version *versions)
{
  struct version *lastp = versions;

  if (versions == NULL)
    return;

  /* Convert into a simple single-linked list.  */
  versions = versions->next;
  assert (versions != NULL);
  lastp->next = NULL;

  do
    {
      struct version *oldp;

      add_id_list (versions->versionname, versions->local_names, true);
      add_id_list (versions->versionname, versions->global_names, false);

      oldp = versions;
      versions = versions->next;
    }
  while (versions != NULL);
}

