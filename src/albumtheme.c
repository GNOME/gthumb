
/*  A Bison parser, made from albumtheme.y
    by GNU Bison version 1.28  */

#define YYBISON 1		/* Identify Bison output.  */

#define	IF	257
#define	ELSE	258
#define	ELSE_IF	259
#define	END	260
#define	END_TAG	261
#define	NAME	262
#define	STRING	263
#define	NUMBER	264
#define	HEADER	265
#define	FOOTER	266
#define	IMAGE	267
#define	IMAGE_LINK	268
#define	IMAGE_IDX	269
#define	IMAGE_DIM	270
#define	IMAGES	271
#define	FILENAME	272
#define	FILEPATH	273
#define	FILESIZE	274
#define	COMMENT	275
#define	PAGE_LINK	276
#define	PAGE_IDX	277
#define	PAGES	278
#define	TABLE	279
#define	DATE	280
#define	TEXT	281
#define	TEXT_END	282
#define	EXIF_EXPOSURE_TIME	283
#define	EXIF_EXPOSURE_MODE	284
#define	EXIF_FLASH	285
#define	EXIF_SHUTTER_SPEED	286
#define	EXIF_APERTURE_VALUE	287
#define	EXIF_FOCAL_LENGTH	288
#define	EXIF_DATE_TIME	289
#define	EXIF_CAMERA_MODEL	290
#define	SET_VAR	291
#define	HTML	292
#define	BOOL_OP	293
#define	COMPARE	294
#define	UNARY_OP	295

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
#include "albumtheme-private.h"

void yyerror (char *fmt, ...);
int yywrap (void);
int yylex (void);

#define YY_NO_UNPUT


#line 34 "albumtheme.y"
typedef union
{
  char *text;
  int ivalue;
  GthVar *var;
  GthTag *tag;
  GthExpr *expr;
  GList *list;
  GthCondition *cond;
}
YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		93
#define	YYFLAG		-32768
#define	YYNTBASE	49

#define YYTRANSLATE(x) ((unsigned)(x) <= 295 ? yytranslate[x] : 63)

static const char yytranslate[] = { 0,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 43, 48, 2, 2, 2, 2, 2, 45,
  46, 2, 41, 2, 42, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  47, 2, 2, 2, 2, 2, 2, 2, 2, 2,
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
  2, 2, 2, 2, 2, 1, 3, 4, 5, 6,
  7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
  37, 38, 39, 40, 44
};

#if YYDEBUG != 0
static const short yyprhs[] = { 0,
  0, 2, 5, 8, 15, 20, 21, 24, 28, 32,
  33, 37, 40, 41, 44, 47, 51, 55, 59, 63,
  67, 70, 73, 76, 78, 80, 84, 88, 90, 92,
  94, 96, 98, 100, 102, 104, 106, 108, 110, 112,
  114, 116, 118, 120, 122, 124, 126, 128, 130, 132,
  134, 136, 138, 141, 142, 146, 152
};

static const short yyrhs[] = { 50,
  0, 38, 50, 0, 59, 50, 0, 51, 50, 52,
  54, 56, 50, 0, 58, 38, 28, 50, 0, 0,
  1, 50, 0, 3, 57, 7, 0, 53, 50, 52,
  0, 0, 5, 57, 7, 0, 55, 50, 0, 0,
  4, 7, 0, 6, 7, 0, 45, 57, 46, 0,
  57, 40, 57, 0, 57, 41, 57, 0, 57, 42,
  57, 0, 57, 39, 57, 0, 41, 57, 0, 42,
  57, 0, 43, 57, 0, 8, 0, 10, 0, 27,
  61, 7, 0, 60, 61, 7, 0, 11, 0, 12,
  0, 13, 0, 14, 0, 15, 0, 16, 0, 17,
  0, 18, 0, 19, 0, 20, 0, 21, 0, 22,
  0, 23, 0, 24, 0, 25, 0, 26, 0, 29,
  0, 30, 0, 31, 0, 32, 0, 33, 0, 34,
  0, 35, 0, 36, 0, 37, 0, 62, 61, 0,
  0, 8, 47, 57, 0, 8, 47, 48, 57, 48,
  0, 8, 0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
  96, 106, 111, 115, 128, 136, 140, 147, 152, 157,
  162, 167, 179, 183, 186, 189, 193, 206, 219, 232,
  245, 249, 254, 259, 266, 273, 278, 283, 284, 285,
  286, 287, 288, 289, 290, 291, 292, 293, 294, 295,
  296, 297, 298, 299, 300, 301, 302, 303, 304, 305,
  306, 307, 310, 314, 319, 324, 329
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char *const yytname[] =
  { "$", "error", "$undefined.", "IF", "ELSE",
  "ELSE_IF", "END", "END_TAG", "NAME", "STRING", "NUMBER", "HEADER", "FOOTER",
    "IMAGE",
  "IMAGE_LINK", "IMAGE_IDX", "IMAGE_DIM", "IMAGES", "FILENAME", "FILEPATH",
    "FILESIZE",
  "COMMENT", "PAGE_LINK", "PAGE_IDX", "PAGES", "TABLE", "DATE", "TEXT",
    "TEXT_END", "EXIF_EXPOSURE_TIME",
  "EXIF_EXPOSURE_MODE", "EXIF_FLASH", "EXIF_SHUTTER_SPEED",
    "EXIF_APERTURE_VALUE",
  "EXIF_FOCAL_LENGTH", "EXIF_DATE_TIME", "EXIF_CAMERA_MODEL", "SET_VAR",
    "HTML", "BOOL_OP",
  "COMPARE", "'+'", "'-'", "'!'", "UNARY_OP", "'('", "')'", "'='", "'\\\"'",
    "all", "document",
  "gthumb_if", "opt_if_list", "gthumb_else_if", "opt_else", "gthumb_else",
    "gthumb_end",
  "expr", "gthumb_text_tag", "gthumb_tag", "tag_name", "arg_list", "arg", NULL
};
#endif

static const short yyr1[] = { 0,
  49, 50, 50, 50, 50, 50, 50, 51, 52, 52,
  53, 54, 54, 55, 56, 57, 57, 57, 57, 57,
  57, 57, 57, 57, 57, 58, 59, 60, 60, 60,
  60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
  60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
  60, 60, 61, 61, 62, 62, 62
};

static const short yyr2[] = { 0,
  1, 2, 2, 6, 4, 0, 2, 3, 3, 0,
  3, 2, 0, 2, 2, 3, 3, 3, 3, 3,
  2, 2, 2, 1, 1, 3, 3, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 2, 0, 3, 5, 1
};

static const short yydefact[] = { 0,
  0, 0, 28, 29, 30, 31, 32, 33, 34, 35,
  36, 37, 38, 39, 40, 41, 42, 43, 54, 44,
  45, 46, 47, 48, 49, 50, 51, 52, 0, 1,
  0, 0, 0, 54, 7, 24, 25, 0, 0, 0,
  0, 0, 57, 0, 54, 2, 10, 0, 3, 0,
  21, 22, 23, 0, 8, 0, 0, 0, 0, 0,
  26, 53, 0, 13, 0, 0, 27, 16, 20, 17,
  18, 19, 0, 55, 0, 0, 0, 0, 10, 5,
  0, 11, 14, 0, 0, 12, 9, 56, 15, 4,
  0, 0, 0
};

static const short yydefgoto[] = { 91,
  30, 31, 64, 65, 77, 78, 85, 42, 32, 33,
  34, 44, 45
};

static const short yypact[] = { 114,
  75, 1, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768,
  -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, 0, -32768,
  -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, 75, -32768,
  152, -28, 75, 0, -32768, -32768, -32768, 1, 1, 1,
  1, 8, -35, 7, 0, -32768, 22, 3, -32768, 26,
  -32768, -32768, -32768, 28, -32768, 1, 1, 1, 1, -7,
  -32768, -32768, 1, 33, 152, 75, -32768, -32768, -24, -21,
  -32768, -32768, 1, 79, 12, 38, 34, 190, 22, -32768,
  18, -32768, -32768, 48, 75, -32768, -32768, -32768, -32768, -32768,
  56, 61, -32768
};

static const short yypgoto[] = { -32768,
  -1, -32768, -17, -32768, -32768, -32768, -32768, -34, -32768, -32768,
  -32768, -32, -32768
};


#define	YYLAST		228


static const short yytable[] = { 35,
  36, 50, 37, 51, 52, 53, 54, 43, 36, 48,
  37, 60, 62, 61, 55, 57, 58, 59, 82, 58,
  59, 69, 70, 71, 72, 74, 63, 46, 75, 47,
  66, 49, 67, 38, 39, 40, 76, 41, 81, 84,
  73, 38, 39, 40, 83, 41, 56, 57, 58, 59,
  56, 57, 58, 59, 89, 92, 56, 57, 58, 59,
  93, 87, 0, 79, 80, 88, 56, 57, 58, 59,
  0, 0, 0, 68, -6, 1, 86, 2, -6, -6,
  -6, 0, 0, 90, 0, 3, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
  18, 19, 0, 20, 21, 22, 23, 24, 25, 26,
  27, 28, 29, -6, 1, 0, 2, 56, 57, 58,
  59, 0, 0, 0, 3, 4, 5, 6, 7, 8,
  9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 0, 20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 1, 0, 2, -6, -6, -6, 0, 0,
  0, 0, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 0,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
  1, 0, 2, 0, 0, -6, 0, 0, 0, 0,
  3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 0, 20, 21,
  22, 23, 24, 25, 26, 27, 28, 29
};

static const short yycheck[] = { 1,
  8, 34, 10, 38, 39, 40, 41, 8, 8, 38,
  10, 47, 45, 7, 7, 40, 41, 42, 7, 41,
  42, 56, 57, 58, 59, 60, 5, 29, 63, 31,
  28, 33, 7, 41, 42, 43, 4, 45, 73, 6,
  48, 41, 42, 43, 7, 45, 39, 40, 41, 42,
  39, 40, 41, 42, 7, 0, 39, 40, 41, 42,
  0, 79, -1, 65, 66, 48, 39, 40, 41, 42,
  -1, -1, -1, 46, 0, 1, 78, 3, 4, 5,
  6, -1, -1, 85, -1, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  26, 27, -1, 29, 30, 31, 32, 33, 34, 35,
  36, 37, 38, 0, 1, -1, 3, 39, 40, 41,
  42, -1, -1, -1, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, -1, 29, 30, 31, 32, 33, 34, 35, 36,
  37, 38, 1, -1, 3, 4, 5, 6, -1, -1,
  -1, -1, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 20, 21, 22, 23, 24, 25, 26, 27, -1,
  29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
  1, -1, 3, -1, -1, 6, -1, -1, -1, -1,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, -1, 29, 30,
  31, 32, 33, 34, 35, 36, 37, 38
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

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

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0				/* No need for malloc.h, which pollutes the namespace;
				   instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
#pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux			/* haible@ilog.fr says this works for HPUX 9.05 and up,
				   and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int yychar;			/*  the lookahead symbol                */
YYSTYPE yylval;			/*  the semantic value of the           */
				/*  lookahead symbol                    */

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead     */
				/*  symbol                              */
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace     */
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else /* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse (YYPARSE_PARAM_ARG)
  YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;		/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short yyssa[YYINITDEPTH];	/*  the state stack                     */
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack            */

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack                  */
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return         */
  /*  semantic values from the action     */
  /*  routines                            */

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf (stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
         the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
         but that might be undefined if yyoverflow is a macro.  */
      yyoverflow ("parser stack overflow",
		  &yyss1, size * sizeof (*yyssp),
		  &yyvs1, size * sizeof (*yyvsp),
		  &yyls1, size * sizeof (*yylsp), &yystacksize);
#else
      yyoverflow ("parser stack overflow",
		  &yyss1, size * sizeof (*yyssp),
		  &yyvs1, size * sizeof (*yyvsp), &yystacksize);
#endif

      yyss = yyss1;
      yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror ("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *) yyss, (char *) yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *) yyvs, (char *) yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *) yyls, (char *) yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf (stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf (stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
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
#if YYDEBUG != 0
      if (yydebug)
	fprintf (stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf (stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
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

#if YYDEBUG != 0
  if (yydebug)
    fprintf (stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1 - yylen];	/* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ", yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn)
    {

    case 1:
#line 96 "albumtheme.y"
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
#line 106 "albumtheme.y"
      {
	yyval.list =
	  g_list_prepend (yyvsp[0].list, gth_tag_new_html (yyvsp[-1].text));
	g_free (yyvsp[-1].text);
	;
	break;
      }
    case 3:
#line 111 "albumtheme.y"
      {
	yyval.list = g_list_prepend (yyvsp[0].list, yyvsp[-1].tag);
	;
	break;
      }
    case 4:
#line 115 "albumtheme.y"
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
#line 128 "albumtheme.y"
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
#line 136 "albumtheme.y"
      {
	yyval.list = NULL;
	;
	break;
      }
    case 7:
#line 140 "albumtheme.y"
      {
	if (yyvsp[0].list != NULL)
	  gth_parsed_doc_free (yyvsp[0].list);
	yyval.list = NULL;
	;
	break;
      }
    case 8:
#line 147 "albumtheme.y"
      {
	yyval.cond = gth_condition_new (yyvsp[-1].expr);
	;
	break;
      }
    case 9:
#line 152 "albumtheme.y"
      {
	gth_condition_add_document (yyvsp[-2].cond, yyvsp[-1].list);
	yyval.list = g_list_prepend (yyvsp[0].list, yyvsp[-2].cond);
	;
	break;
      }
    case 10:
#line 157 "albumtheme.y"
      {
	yyval.list = NULL;
	;
	break;
      }
    case 11:
#line 162 "albumtheme.y"
      {
	yyval.cond = gth_condition_new (yyvsp[-1].expr);
	;
	break;
      }
    case 12:
#line 167 "albumtheme.y"
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
#line 179 "albumtheme.y"
      {
	yyval.cond = NULL;
	;
	break;
      }
    case 16:
#line 189 "albumtheme.y"
      {
	yyval.expr = yyvsp[-1].expr;
	;
	break;
      }
    case 17:
#line 193 "albumtheme.y"
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
#line 206 "albumtheme.y"
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
#line 219 "albumtheme.y"
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
#line 232 "albumtheme.y"
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
    case 21:
#line 245 "albumtheme.y"
      {
	yyval.expr = yyvsp[0].expr;
	;
	break;
      }
    case 22:
#line 249 "albumtheme.y"
      {
	gth_expr_push_op (yyvsp[0].expr, GTH_OP_NEG);
	yyval.expr = yyvsp[0].expr;
	;
	break;
      }
    case 23:
#line 254 "albumtheme.y"
      {
	gth_expr_push_op (yyvsp[0].expr, GTH_OP_NOT);
	yyval.expr = yyvsp[0].expr;
	;
	break;
      }
    case 24:
#line 259 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	gth_expr_push_var (e, yyvsp[0].text);
	g_free (yyvsp[0].text);
	yyval.expr = e;
	;
	break;
      }
    case 25:
#line 266 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	gth_expr_push_constant (e, yyvsp[0].ivalue);
	yyval.expr = e;
	;
	break;
      }
    case 26:
#line 273 "albumtheme.y"
      {
	yyval.tag = gth_tag_new (yyvsp[-2].ivalue, yyvsp[-1].list);
	;
	break;
      }
    case 27:
#line 278 "albumtheme.y"
      {
	yyval.tag = gth_tag_new (yyvsp[-2].ivalue, yyvsp[-1].list);
	;
	break;
      }
    case 28:
#line 283 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 29:
#line 284 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 30:
#line 285 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 31:
#line 286 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 32:
#line 287 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 33:
#line 288 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 34:
#line 289 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 35:
#line 290 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 36:
#line 291 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 37:
#line 292 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 38:
#line 293 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 39:
#line 294 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 40:
#line 295 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 41:
#line 296 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 42:
#line 297 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 43:
#line 298 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 44:
#line 299 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 45:
#line 300 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 46:
#line 301 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 47:
#line 302 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 48:
#line 303 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 49:
#line 304 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 50:
#line 305 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 51:
#line 306 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 52:
#line 307 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
	break;
      }
    case 53:
#line 310 "albumtheme.y"
      {
	yyval.list = g_list_prepend (yyvsp[0].list, yyvsp[-1].var);
	;
	break;
      }
    case 54:
#line 314 "albumtheme.y"
      {
	yyval.list = NULL;
	;
	break;
      }
    case 55:
#line 319 "albumtheme.y"
      {
	yyval.var = gth_var_new_expression (yyvsp[-2].text, yyvsp[0].expr);
	g_free (yyvsp[-2].text);
	;
	break;
      }
    case 56:
#line 324 "albumtheme.y"
      {
	yyval.var = gth_var_new_expression (yyvsp[-4].text, yyvsp[-1].expr);
	g_free (yyvsp[-4].text);
	;
	break;
      }
    case 57:
#line 329 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	gth_expr_push_constant (e, 1);
	yyval.var = gth_var_new_expression (yyvsp[0].text, e);
	g_free (yyvsp[0].text);
	;
	break;
      }
    }
  /* the action file gets copied in in place of this dollarsign */
#line 543 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp - 1)->last_line;
      yylsp->last_column = (yylsp - 1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp + yylen - 1)->last_line;
      yylsp->last_column = (yylsp + yylen - 1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:			/* here on detecting error */

  if (!yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof (yytname) / sizeof (char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen (yytname[x]) + 15, count++;
	  msg = (char *) malloc (size + 15);
	  if (msg != 0)
	    {
	      strcpy (msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof (yytname) / sizeof (char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat (msg, count == 0 ? ", expecting `" : " or `");
			strcat (msg, yytname[x]);
			strcat (msg, "'");
			count++;
		      }
		}
	      yyerror (msg);
	      free (msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("parse error");
    }

  goto yyerrlab1;
yyerrlab1:			/* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf (stderr, "Discarding token %d (%s).\n", yychar,
		 yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:			/* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];	/* If its default is to accept any token, ok.  Otherwise pop it. */
  if (yyn)
    goto yydefault;
#endif

yyerrpop:			/* pop the current state because it cannot handle the error token */

  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

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

#if YYDEBUG != 0
  if (yydebug)
    fprintf (stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}

#line 337 "albumtheme.y"


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
