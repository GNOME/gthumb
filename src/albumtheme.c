/* A Bison parser, made from albumtheme.y
   by GNU bison 1.35.  */

#define YYBISON 1		/* Identify Bison output.  */

# define	IF	257
# define	ELSE	258
# define	ELSE_IF	259
# define	END	260
# define	END_TAG	261
# define	NAME	262
# define	STRING	263
# define	NUMBER	264
# define	HEADER	265
# define	FOOTER	266
# define	IMAGE	267
# define	IMAGE_LINK	268
# define	IMAGE_IDX	269
# define	IMAGE_DIM	270
# define	IMAGES	271
# define	FILENAME	272
# define	FILEPATH	273
# define	FILESIZE	274
# define	COMMENT	275
# define	PAGE_LINK	276
# define	PAGE_IDX	277
# define	PAGES	278
# define	TABLE	279
# define	DATE	280
# define	TEXT	281
# define	TEXT_END	282
# define	EXIF_EXPOSURE_TIME	283
# define	EXIF_EXPOSURE_MODE	284
# define	EXIF_FLASH	285
# define	EXIF_SHUTTER_SPEED	286
# define	EXIF_APERTURE_VALUE	287
# define	EXIF_FOCAL_LENGTH	288
# define	EXIF_DATE_TIME	289
# define	EXIF_CAMERA_MODEL	290
# define	SET_VAR	291
# define	EVAL	292
# define	HTML	293
# define	BOOL_OP	294
# define	COMPARE	295
# define	UNARY_OP	296

#line 1 "albumtheme.y"

/*
 *  GThumb
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "albumtheme-private.h"

void yyerror (char *fmt, ...);
int yywrap (void);
int yylex (void);

#define YY_NO_UNPUT


#line 35 "albumtheme.y"
#ifndef YYSTYPE
typedef union
{
  char *text;
  int ivalue;
  GthVar *var;
  GthTag *tag;
  GthExpr *expr;
  GList *list;
  GthCondition *cond;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		98
#define	YYFLAG		-32768
#define	YYNTBASE	52

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 296 ? yytranslate[x] : 66)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] = {
  0, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 46, 51, 2, 2, 2, 2, 2,
  48, 49, 44, 42, 2, 43, 2, 45, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 50, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 1, 3, 4, 5,
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
  36, 37, 38, 39, 40, 41, 47
};

#if YYDEBUG
static const short yyprhs[] = {
  0, 0, 2, 5, 8, 15, 20, 21, 24, 28,
  32, 33, 37, 40, 41, 44, 47, 51, 55, 59,
  63, 67, 71, 75, 78, 81, 84, 86, 88, 92,
  96, 98, 100, 102, 104, 106, 108, 110, 112, 114,
  116, 118, 120, 122, 124, 126, 128, 130, 132, 134,
  136, 138, 140, 142, 144, 146, 148, 151, 152, 156,
  162
};
static const short yyrhs[] = {
  53, 0, 39, 53, 0, 62, 53, 0, 54, 53,
  55, 57, 59, 53, 0, 61, 39, 28, 53, 0,
  0, 1, 53, 0, 3, 60, 7, 0, 56, 53,
  55, 0, 0, 5, 60, 7, 0, 58, 53, 0,
  0, 4, 7, 0, 6, 7, 0, 48, 60, 49,
  0, 60, 41, 60, 0, 60, 42, 60, 0, 60,
  43, 60, 0, 60, 44, 60, 0, 60, 45, 60,
  0, 60, 40, 60, 0, 42, 60, 0, 43, 60,
  0, 46, 60, 0, 8, 0, 10, 0, 27, 64,
  7, 0, 63, 64, 7, 0, 11, 0, 12, 0,
  13, 0, 14, 0, 15, 0, 16, 0, 17, 0,
  18, 0, 19, 0, 20, 0, 21, 0, 22, 0,
  23, 0, 24, 0, 25, 0, 26, 0, 29, 0,
  30, 0, 31, 0, 32, 0, 33, 0, 34, 0,
  35, 0, 36, 0, 37, 0, 38, 0, 65, 64,
  0, 0, 8, 50, 60, 0, 8, 50, 51, 60,
  51, 0, 8, 0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] = {
  0, 98, 108, 113, 117, 130, 138, 142, 149, 154,
  159, 164, 169, 181, 186, 189, 192, 196, 209, 222,
  235, 248, 261, 274, 278, 283, 288, 295, 302, 307,
  312, 313, 314, 315, 316, 317, 318, 319, 320, 321,
  322, 323, 324, 325, 326, 327, 328, 329, 330, 331,
  332, 333, 334, 335, 336, 337, 340, 344, 349, 354,
  359
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] = {
  "$", "error", "$undefined.", "IF", "ELSE", "ELSE_IF", "END", "END_TAG",
  "NAME", "STRING", "NUMBER", "HEADER", "FOOTER", "IMAGE", "IMAGE_LINK",
  "IMAGE_IDX", "IMAGE_DIM", "IMAGES", "FILENAME", "FILEPATH", "FILESIZE",
  "COMMENT", "PAGE_LINK", "PAGE_IDX", "PAGES", "TABLE", "DATE", "TEXT",
  "TEXT_END", "EXIF_EXPOSURE_TIME", "EXIF_EXPOSURE_MODE", "EXIF_FLASH",
  "EXIF_SHUTTER_SPEED", "EXIF_APERTURE_VALUE", "EXIF_FOCAL_LENGTH",
  "EXIF_DATE_TIME", "EXIF_CAMERA_MODEL", "SET_VAR", "EVAL", "HTML",
  "BOOL_OP", "COMPARE", "'+'", "'-'", "'*'", "'/'", "'!'", "UNARY_OP",
  "'('", "')'", "'='", "'\\\"'", "all", "document", "gthumb_if",
  "opt_if_list", "gthumb_else_if", "opt_else", "gthumb_else",
  "gthumb_end", "expr", "gthumb_text_tag", "gthumb_tag", "tag_name",
  "arg_list", "arg", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] = {
  0, 52, 53, 53, 53, 53, 53, 53, 54, 55,
  55, 56, 57, 57, 58, 59, 60, 60, 60, 60,
  60, 60, 60, 60, 60, 60, 60, 60, 61, 62,
  63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
  63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
  63, 63, 63, 63, 63, 63, 64, 64, 65, 65,
  65
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] = {
  0, 1, 2, 2, 6, 4, 0, 2, 3, 3,
  0, 3, 2, 0, 2, 2, 3, 3, 3, 3,
  3, 3, 3, 2, 2, 2, 1, 1, 3, 3,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 2, 0, 3, 5,
  1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] = {
  0, 0, 0, 30, 31, 32, 33, 34, 35, 36,
  37, 38, 39, 40, 41, 42, 43, 44, 45, 57,
  46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
  0, 1, 0, 0, 0, 57, 7, 26, 27, 0,
  0, 0, 0, 0, 60, 0, 57, 2, 10, 0,
  3, 0, 23, 24, 25, 0, 8, 0, 0, 0,
  0, 0, 0, 0, 28, 56, 0, 13, 0, 0,
  29, 16, 22, 17, 18, 19, 20, 21, 0, 58,
  0, 0, 0, 0, 10, 5, 0, 11, 14, 0,
  0, 12, 9, 59, 15, 4, 0, 0, 0
};

static const short yydefgoto[] = {
  96, 31, 32, 67, 68, 82, 83, 90, 43, 33,
  34, 35, 45, 46
};

static const short yypact[] = {
  120, 80, 0, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
  -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -6,
  -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
    -32768,
  80, -32768, 159, -21, 80, -6, -32768, -32768, -32768, 0,
  0, 0, 0, 15, -39, 14, -6, -32768, 19, -9,
  -32768, 18, -32768, -32768, -32768, 199, -32768, 0, 0, 0,
  0, 0, 0, -7, -32768, -32768, 0, 22, 159, 80,
  -32768, -32768, 208, -38, -32768, -32768, -32768, -32768, 0, 84,
  21, 20, 24, 198, 19, -32768, -28, -32768, -32768, 25,
  80, -32768, -32768, -32768, -32768, -32768, 34, 37, -32768
};

static const short yypgoto[] = {
  -32768, -1, -32768, -46, -32768, -32768, -32768, -32768, 12, -32768,
  -32768, -32768, -26, -32768
};


#define	YYLAST		253


static const short yytable[] = {
  36, 37, 44, 38, 59, 60, 61, 62, 37, 51,
  38, 63, 57, 58, 59, 60, 61, 62, 49, 69,
  65, 64, 56, 93, 66, 70, 81, 88, 87, 47,
  89, 48, 94, 50, 97, 39, 40, 98, 92, 41,
  0, 42, 39, 40, 78, 0, 41, 0, 42, 0,
  0, 52, 53, 54, 55, 57, 58, 59, 60, 61,
  62, 57, 58, 59, 60, 61, 62, 84, 85, 72,
  73, 74, 75, 76, 77, 79, 0, 0, 80, 0,
  -6, 1, 91, 2, -6, -6, -6, 0, 0, 95,
  86, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 0, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  -6, 1, 0, 2, 57, 58, 59, 60, 61, 62,
  0, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 0, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  1, 0, 2, -6, -6, -6, 0, 0, 0, 0,
  3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 0, 20, 21,
  22, 23, 24, 25, 26, 27, 28, 29, 30, 1,
  0, 2, 0, 0, -6, 0, 0, 0, 0, 3,
  4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
  14, 15, 16, 17, 18, 19, 0, 20, 21, 22,
  23, 24, 25, 26, 27, 28, 29, 30, 0, 57,
  58, 59, 60, 61, 62, 0, 0, 0, 71, 58,
  59, 60, 61, 62
};

static const short yycheck[] = {
  1, 8, 8, 10, 42, 43, 44, 45, 8, 35,
  10, 50, 40, 41, 42, 43, 44, 45, 39, 28,
  46, 7, 7, 51, 5, 7, 4, 7, 7, 30,
  6, 32, 7, 34, 0, 42, 43, 0, 84, 46,
  -1, 48, 42, 43, 51, -1, 46, -1, 48, -1,
  -1, 39, 40, 41, 42, 40, 41, 42, 43, 44,
  45, 40, 41, 42, 43, 44, 45, 68, 69, 57,
  58, 59, 60, 61, 62, 63, -1, -1, 66, -1,
  0, 1, 83, 3, 4, 5, 6, -1, -1, 90,
  78, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, -1, 29,
  30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
  0, 1, -1, 3, 40, 41, 42, 43, 44, 45,
  -1, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, -1, 29,
  30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
  1, -1, 3, 4, 5, 6, -1, -1, -1, -1,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, -1, 29, 30,
  31, 32, 33, 34, 35, 36, 37, 38, 39, 1,
  -1, 3, -1, -1, 6, -1, -1, -1, -1, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
  22, 23, 24, 25, 26, 27, -1, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, -1, 40,
  41, 42, 43, 44, 45, -1, -1, -1, 49, 41,
  42, 43, 44, 45
};

/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison/bison.simple"

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

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

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if ! defined (yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h>		/* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || defined (YYERROR_VERBOSE) */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
# if YYLSP_NEEDED
  YYLTYPE yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
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
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;	\
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h>		/* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
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
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, &yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval, &yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval)
#  endif
# endif	/* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX			yylex ()
#endif /* !YYPURE */


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h>		/* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
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

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

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
#endif

#line 315 "/usr/share/bison/bison.simple"


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int yychar;						\
							\
/* The semantic value of the lookahead symbol. */	\
YYSTYPE yylval;						\
							\
/* Number of parse errors so far.  */			\
int yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
YYLTYPE yylloc;
#else
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif /* !YYPURE */
  int
yyparse (YYPARSE_PARAM_ARG)
  YYPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif /* !YYPURE */
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
# define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  YYSIZE_T yystacksize = YYINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
#if YYLSP_NEEDED
  YYLTYPE yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
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
#if YYLSP_NEEDED
  yylsp = yyls;
#endif
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

  if (yyssp >= yyss + yystacksize - 1)
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
	   data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
	YYLTYPE *yyls1 = yyls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp), &yystacksize);
	yyls = yyls1;
# else
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp), &yystacksize);
# endif
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (!yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
# if YYLSP_NEEDED
	YYSTACK_RELOCATE (yyls);
# endif
# undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
#if YYLSP_NEEDED
      yylsp = yyls + yysize - 1;
#endif

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyssp >= yyss + yystacksize - 1)
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
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG
      /* We have to keep this `#if YYDEBUG', since we use variables
         which are defined only if `YYDEBUG' is set.  */
      if (yydebug)
	{
	  YYFPRINTF (stderr, "Next token is %d (%s",
		     yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
# endif
	  YYFPRINTF (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
     New state is final state => don't bother to shift,
     just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

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

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1 - yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  yyloc = yylsp[1 - yylen];
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] > 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif

  switch (yyn)
    {

    case 1:
#line 98 "albumtheme.y"
      {
	yy_parsed_doc = yyvsp[0].list;

	if (yy_parsed_doc == NULL)
	  YYABORT;
	else
	  YYACCEPT;
	;
	break;
      }
    case 2:
#line 108 "albumtheme.y"
      {
	yyval.list =
	  g_list_prepend (yyvsp[0].list, gth_tag_new_html (yyvsp[-1].text));
	g_free (yyvsp[-1].text);
	;
	break;
      }
    case 3:
#line 113 "albumtheme.y"
      {
	yyval.list = g_list_prepend (yyvsp[0].list, yyvsp[-1].tag);
	;
	break;
      }
    case 4:
#line 117 "albumtheme.y"
      {
	GList *cond_list;
	GthTag *tag;

	gth_condition_add_document (yyvsp[-5].cond, yyvsp[-4].list);
	cond_list = g_list_prepend (yyvsp[-3].list, yyvsp[-5].cond);
	if (yyvsp[-2].cond != NULL)
	  cond_list = g_list_append (cond_list, yyvsp[-2].cond);

	tag = gth_tag_new_condition (cond_list);
	yyval.list = g_list_prepend (yyvsp[0].list, tag);
	;
	break;
      }
    case 5:
#line 130 "albumtheme.y"
      {
	GthTag *tag = gth_tag_new_html (yyvsp[-2].text);
	GList *child_doc = g_list_append (NULL, tag);
	gth_tag_add_document (yyvsp[-3].tag, child_doc);
	yyval.list = g_list_prepend (yyvsp[0].list, yyvsp[-3].tag);
	g_free (yyvsp[-2].text);
	;
	break;
      }
    case 6:
#line 138 "albumtheme.y"
      {
	yyval.list = NULL;
	;
	break;
      }
    case 7:
#line 142 "albumtheme.y"
      {
	if (yyvsp[0].list != NULL)
	  gth_parsed_doc_free (yyvsp[0].list);
	yyval.list = NULL;
	;
	break;
      }
    case 8:
#line 149 "albumtheme.y"
      {
	yyval.cond = gth_condition_new (yyvsp[-1].expr);
	;
	break;
      }
    case 9:
#line 154 "albumtheme.y"
      {
	gth_condition_add_document (yyvsp[-2].cond, yyvsp[-1].list);
	yyval.list = g_list_prepend (yyvsp[0].list, yyvsp[-2].cond);
	;
	break;
      }
    case 10:
#line 159 "albumtheme.y"
      {
	yyval.list = NULL;
	;
	break;
      }
    case 11:
#line 164 "albumtheme.y"
      {
	yyval.cond = gth_condition_new (yyvsp[-1].expr);
	;
	break;
      }
    case 12:
#line 169 "albumtheme.y"
      {
	GthExpr *else_expr;
	GthCondition *cond;

	else_expr = gth_expr_new ();
	gth_expr_push_constant (else_expr, 1);
	cond = gth_condition_new (else_expr);
	gth_condition_add_document (cond, yyvsp[0].list);

	yyval.cond = cond;
	;
	break;
      }
    case 13:
#line 181 "albumtheme.y"
      {
	yyval.cond = NULL;
	;
	break;
      }
    case 16:
#line 192 "albumtheme.y"
      {
	yyval.expr = yyvsp[-1].expr;
	;
	break;
      }
    case 17:
#line 196 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, yyvsp[-2].expr);
	gth_expr_push_expr (e, yyvsp[0].expr);
	gth_expr_push_op (e, yyvsp[-1].ivalue);

	gth_expr_unref (yyvsp[-2].expr);
	gth_expr_unref (yyvsp[0].expr);

	yyval.expr = e;
	;
	break;
      }
    case 18:
#line 209 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, yyvsp[-2].expr);
	gth_expr_push_expr (e, yyvsp[0].expr);
	gth_expr_push_op (e, GTH_OP_ADD);

	gth_expr_unref (yyvsp[-2].expr);
	gth_expr_unref (yyvsp[0].expr);

	yyval.expr = e;
	;
	break;
      }
    case 19:
#line 222 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, yyvsp[-2].expr);
	gth_expr_push_expr (e, yyvsp[0].expr);
	gth_expr_push_op (e, GTH_OP_SUB);

	gth_expr_unref (yyvsp[-2].expr);
	gth_expr_unref (yyvsp[0].expr);

	yyval.expr = e;
	;
	break;
      }
    case 20:
#line 235 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, yyvsp[-2].expr);
	gth_expr_push_expr (e, yyvsp[0].expr);
	gth_expr_push_op (e, GTH_OP_MUL);

	gth_expr_unref (yyvsp[-2].expr);
	gth_expr_unref (yyvsp[0].expr);

	yyval.expr = e;
	;
	break;
      }
    case 21:
#line 248 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, yyvsp[-2].expr);
	gth_expr_push_expr (e, yyvsp[0].expr);
	gth_expr_push_op (e, GTH_OP_DIV);

	gth_expr_unref (yyvsp[-2].expr);
	gth_expr_unref (yyvsp[0].expr);

	yyval.expr = e;
	;
	break;
      }
    case 22:
#line 261 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, yyvsp[-2].expr);
	gth_expr_push_expr (e, yyvsp[0].expr);
	gth_expr_push_op (e, yyvsp[-1].ivalue);

	gth_expr_unref (yyvsp[-2].expr);
	gth_expr_unref (yyvsp[0].expr);

	yyval.expr = e;
	;
	break;
      }
    case 23:
#line 274 "albumtheme.y"
      {
	yyval.expr = yyvsp[0].expr;
	;
	break;
      }
    case 24:
#line 278 "albumtheme.y"
      {
	gth_expr_push_op (yyvsp[0].expr, GTH_OP_NEG);
	yyval.expr = yyvsp[0].expr;
	;
	break;
      }
    case 25:
#line 283 "albumtheme.y"
      {
	gth_expr_push_op (yyvsp[0].expr, GTH_OP_NOT);
	yyval.expr = yyvsp[0].expr;
	;
	break;
      }
    case 26:
#line 288 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	gth_expr_push_var (e, yyvsp[0].text);
	g_free (yyvsp[0].text);
	yyval.expr = e;
	;
	break;
      }
    case 27:
#line 295 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	gth_expr_push_constant (e, yyvsp[0].ivalue);
	yyval.expr = e;
	;
	break;
      }
    case 28:
#line 302 "albumtheme.y"
      {
	yyval.tag = gth_tag_new (yyvsp[-2].ivalue, yyvsp[-1].list);
	;
	break;
      }
    case 29:
#line 307 "albumtheme.y"
      {
	yyval.tag = gth_tag_new (yyvsp[-2].ivalue, yyvsp[-1].list);
	;
	break;
      }
    case 30:
#line 312 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 31:
#line 313 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 32:
#line 314 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 33:
#line 315 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 34:
#line 316 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 35:
#line 317 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 36:
#line 318 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 37:
#line 319 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 38:
#line 320 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 39:
#line 321 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 40:
#line 322 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 41:
#line 323 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 42:
#line 324 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 43:
#line 325 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 44:
#line 326 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 45:
#line 327 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 46:
#line 328 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 47:
#line 329 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 48:
#line 330 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 49:
#line 331 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 50:
#line 332 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 51:
#line 333 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 52:
#line 334 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 53:
#line 335 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 54:
#line 336 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 55:
#line 337 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 56:
#line 340 "albumtheme.y"
      {
	yyval.list = g_list_prepend (yyvsp[0].list, yyvsp[-1].var);
	;
	break;
      }
    case 57:
#line 344 "albumtheme.y"
      {
	yyval.list = NULL;
	;
	break;
      }
    case 58:
#line 349 "albumtheme.y"
      {
	yyval.var = gth_var_new_expression (yyvsp[-2].text, yyvsp[0].expr);
	g_free (yyvsp[-2].text);
	;
	break;
      }
    case 59:
#line 354 "albumtheme.y"
      {
	yyval.var = gth_var_new_expression (yyvsp[-4].text, yyvsp[-1].expr);
	g_free (yyvsp[-4].text);
	;
	break;
      }
    case 60:
#line 359 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	gth_expr_push_constant (e, 1);
	yyval.var = gth_var_new_expression (yyvsp[0].text, e);
	g_free (yyvsp[0].text);
	;
	break;
      }
    }

#line 705 "/usr/share/bison/bison.simple"


  yyvsp -= yylen;
  yyssp -= yylen;
#if YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;
#if YYLSP_NEEDED
  *++yylsp = yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[YYTRANSLATE (yychar)]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[YYTRANSLATE (yychar)]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx)
		      {
			const char *yyq = !yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (YYERROR_VERBOSE) */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*--------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
yyerrlab1:
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;
      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;


/*-------------------------------------------------------------------.
| yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
yyerrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  yyn = yydefact[yystate];
  if (yyn)
    goto yydefault;
#endif


/*---------------------------------------------------------------.
| yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
yyerrpop:
  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#if YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "Error: state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| yyerrhandle.  |
`--------------*/
yyerrhandle:
  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

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

/*---------------------------------------------.
| yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}

#line 367 "albumtheme.y"


int
yywrap (void)
{
  return 1;
}


void
yyerror (char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  va_end (ap);
}


#include "lex.albumtheme.c"
