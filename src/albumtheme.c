/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
enum yytokentype
{
  END = 258,
  ELSE_IF = 259,
  ELSE = 260,
  IF = 261,
  END_TAG = 262,
  NAME = 263,
  STRING = 264,
  NUMBER = 265,
  HEADER = 266,
  FOOTER = 267,
  LANGUAGE = 268,
  IMAGE = 269,
  IMAGE_LINK = 270,
  IMAGE_IDX = 271,
  IMAGE_DIM = 272,
  IMAGES = 273,
  FILENAME = 274,
  FILEPATH = 275,
  FILESIZE = 276,
  COMMENT = 277,
  PAGE_LINK = 278,
  PAGE_IDX = 279,
  PAGES = 280,
  TABLE = 281,
  DATE = 282,
  TEXT = 283,
  TEXT_END = 284,
  EXIF_EXPOSURE_TIME = 285,
  EXIF_EXPOSURE_MODE = 286,
  EXIF_FLASH = 287,
  EXIF_SHUTTER_SPEED = 288,
  EXIF_APERTURE_VALUE = 289,
  EXIF_FOCAL_LENGTH = 290,
  EXIF_DATE_TIME = 291,
  EXIF_CAMERA_MODEL = 292,
  SET_VAR = 293,
  EVAL = 294,
  HTML = 295,
  BOOL_OP = 296,
  COMPARE = 297,
  UNARY_OP = 298
};
#endif
#define END 258
#define ELSE_IF 259
#define ELSE 260
#define IF 261
#define END_TAG 262
#define NAME 263
#define STRING 264
#define NUMBER 265
#define HEADER 266
#define FOOTER 267
#define LANGUAGE 268
#define IMAGE 269
#define IMAGE_LINK 270
#define IMAGE_IDX 271
#define IMAGE_DIM 272
#define IMAGES 273
#define FILENAME 274
#define FILEPATH 275
#define FILESIZE 276
#define COMMENT 277
#define PAGE_LINK 278
#define PAGE_IDX 279
#define PAGES 280
#define TABLE 281
#define DATE 282
#define TEXT 283
#define TEXT_END 284
#define EXIF_EXPOSURE_TIME 285
#define EXIF_EXPOSURE_MODE 286
#define EXIF_FLASH 287
#define EXIF_SHUTTER_SPEED 288
#define EXIF_APERTURE_VALUE 289
#define EXIF_FOCAL_LENGTH 290
#define EXIF_DATE_TIME 291
#define EXIF_CAMERA_MODEL 292
#define SET_VAR 293
#define EVAL 294
#define HTML 295
#define BOOL_OP 296
#define COMPARE 297
#define UNARY_OP 298




/* Copy the first part of user declarations.  */
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
#line 35 "albumtheme.y"
typedef union YYSTYPE
{
  char *text;
  int ivalue;
  GthVar *var;
  GthTag *tag;
  GthExpr *expr;
  GList *list;
  GthCondition *cond;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 205 "albumtheme.c"
# define yystype YYSTYPE	/* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 217 "albumtheme.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

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
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

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
#define YYFINAL  50
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   261

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  53
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  15
/* YYNRULES -- Number of rules. */
#define YYNRULES  62
/* YYNRULES -- Number of states. */
#define YYNSTATES  99

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   298

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] = {
  0, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 47, 52, 2, 2, 2, 2, 2,
  49, 50, 45, 43, 2, 44, 2, 46, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 51, 2, 2, 2, 2, 2, 2, 2, 2,
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
  2, 2, 2, 2, 2, 2, 1, 2, 3, 4,
  5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
  35, 36, 37, 38, 39, 40, 41, 42, 48
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] = {
  0, 0, 3, 5, 8, 11, 18, 23, 24, 27,
  31, 35, 36, 40, 43, 44, 47, 50, 54, 58,
  62, 66, 70, 74, 78, 81, 84, 87, 89, 91,
  95, 99, 101, 103, 105, 107, 109, 111, 113, 115,
  117, 119, 121, 123, 125, 127, 129, 131, 133, 135,
  137, 139, 141, 143, 145, 147, 149, 151, 153, 156,
  157, 161, 167
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] = {
  54, 0, -1, 55, -1, 40, 55, -1, 64, 55,
  -1, 56, 55, 57, 59, 61, 55, -1, 63, 40,
  29, 55, -1, -1, 1, 55, -1, 6, 62, 7,
  -1, 58, 55, 57, -1, -1, 4, 62, 7, -1,
  60, 55, -1, -1, 5, 7, -1, 3, 7, -1,
  49, 62, 50, -1, 62, 42, 62, -1, 62, 43,
  62, -1, 62, 44, 62, -1, 62, 45, 62, -1,
  62, 46, 62, -1, 62, 41, 62, -1, 43, 62,
  -1, 44, 62, -1, 47, 62, -1, 8, -1, 10,
  -1, 28, 66, 7, -1, 65, 66, 7, -1, 11,
  -1, 12, -1, 13, -1, 14, -1, 15, -1, 16,
  -1, 17, -1, 18, -1, 19, -1, 20, -1, 21,
  -1, 22, -1, 23, -1, 24, -1, 25, -1, 26,
  -1, 27, -1, 30, -1, 31, -1, 32, -1, 33,
  -1, 34, -1, 35, -1, 36, -1, 37, -1, 38,
  -1, 39, -1, 67, 66, -1, -1, 8, 51, 62,
  -1, 8, 51, 52, 62, 52, -1, 8, -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] = {
  0, 99, 99, 109, 114, 118, 131, 139, 143, 150,
  155, 160, 165, 170, 182, 187, 190, 193, 197, 210,
  223, 236, 249, 262, 275, 279, 284, 289, 296, 303,
  308, 313, 314, 315, 316, 317, 318, 319, 320, 321,
  322, 323, 324, 325, 326, 327, 328, 329, 330, 331,
  332, 333, 334, 335, 336, 337, 338, 339, 342, 346,
  351, 356, 361
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] = {
  "$end", "error", "$undefined", "END", "ELSE_IF", "ELSE", "IF", "END_TAG",
  "NAME", "STRING", "NUMBER", "HEADER", "FOOTER", "LANGUAGE", "IMAGE",
  "IMAGE_LINK", "IMAGE_IDX", "IMAGE_DIM", "IMAGES", "FILENAME",
  "FILEPATH", "FILESIZE", "COMMENT", "PAGE_LINK", "PAGE_IDX", "PAGES",
  "TABLE", "DATE", "TEXT", "TEXT_END", "EXIF_EXPOSURE_TIME",
  "EXIF_EXPOSURE_MODE", "EXIF_FLASH", "EXIF_SHUTTER_SPEED",
  "EXIF_APERTURE_VALUE", "EXIF_FOCAL_LENGTH", "EXIF_DATE_TIME",
  "EXIF_CAMERA_MODEL", "SET_VAR", "EVAL", "HTML", "BOOL_OP", "COMPARE",
  "'+'", "'-'", "'*'", "'/'", "'!'", "UNARY_OP", "'('", "')'", "'='",
  "'\"'", "$accept", "all", "document", "gthumb_if", "opt_if_list",
  "gthumb_else_if", "opt_else", "gthumb_else", "gthumb_end", "expr",
  "gthumb_text_tag", "gthumb_tag", "tag_name", "arg_list", "arg", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] = {
  0, 256, 257, 258, 259, 260, 261, 262, 263, 264,
  265, 266, 267, 268, 269, 270, 271, 272, 273, 274,
  275, 276, 277, 278, 279, 280, 281, 282, 283, 284,
  285, 286, 287, 288, 289, 290, 291, 292, 293, 294,
  295, 296, 297, 43, 45, 42, 47, 33, 298, 40,
  41, 61, 34
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] = {
  0, 53, 54, 55, 55, 55, 55, 55, 55, 56,
  57, 57, 58, 59, 59, 60, 61, 62, 62, 62,
  62, 62, 62, 62, 62, 62, 62, 62, 62, 63,
  64, 65, 65, 65, 65, 65, 65, 65, 65, 65,
  65, 65, 65, 65, 65, 65, 65, 65, 65, 65,
  65, 65, 65, 65, 65, 65, 65, 65, 66, 66,
  67, 67, 67
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] = {
  0, 2, 1, 2, 2, 6, 4, 0, 2, 3,
  3, 0, 3, 2, 0, 2, 2, 3, 3, 3,
  3, 3, 3, 3, 2, 2, 2, 1, 1, 3,
  3, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 2, 0,
  3, 5, 1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] = {
  0, 0, 0, 31, 32, 33, 34, 35, 36, 37,
  38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  59, 48, 49, 50, 51, 52, 53, 54, 55, 56,
  57, 0, 0, 2, 0, 0, 0, 59, 8, 27,
  28, 0, 0, 0, 0, 0, 62, 0, 59, 3,
  1, 11, 0, 4, 0, 24, 25, 26, 0, 9,
  0, 0, 0, 0, 0, 0, 0, 29, 58, 0,
  14, 0, 0, 30, 17, 23, 18, 19, 20, 21,
  22, 0, 60, 0, 0, 0, 0, 11, 6, 0,
  12, 15, 0, 0, 13, 10, 61, 16, 5
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] = {
  -1, 32, 33, 34, 70, 71, 85, 86, 93, 45,
  35, 36, 37, 47, 48
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -45
static const short yypact[] = {
  124, 83, 0, -45, -45, -45, -45, -45, -45, -45,
  -45, -45, -45, -45, -45, -45, -45, -45, -45, -45,
  -6, -45, -45, -45, -45, -45, -45, -45, -45, -45,
  -45, 83, 4, -45, 164, -35, 83, -6, -45, -45,
  -45, 0, 0, 0, 0, 16, -44, 2, -6, -45,
  -45, 27, 3, -45, 31, -45, -45, -45, 205, -45,
  0, 0, 0, 0, 0, 0, -7, -45, -45, 0,
  29, 164, 83, -45, -45, -18, -25, -45, -45, -45,
  -45, 0, 215, 22, 32, 38, 204, 27, -45, -30,
  -45, -45, 39, 83, -45, -45, -45, -45, -45
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] = {
  -45, -45, -1, -45, -39, -45, -45, -45, -45, 12,
  -45, -45, -45, -31, -45
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -8
static const yysigned_char yytable[] = {
  38, 39, 46, 40, 50, 52, 54, 66, 39, 67,
  40, 60, 61, 62, 63, 64, 65, 68, 62, 63,
  64, 65, 96, 59, 61, 62, 63, 64, 65, 90,
  49, 69, 72, 51, 84, 53, 41, 42, 73, 91,
  43, 92, 44, 41, 42, 81, 97, 43, 95, 44,
  0, 0, 0, 55, 56, 57, 58, 60, 61, 62,
  63, 64, 65, 60, 61, 62, 63, 64, 65, 0,
  87, 88, 75, 76, 77, 78, 79, 80, 82, 0,
  0, 83, 0, -7, 1, 94, -7, -7, -7, 2,
  0, 0, 98, 89, 3, 4, 5, 6, 7, 8,
  9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 20, 0, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, -7, 1, 0, 0, 0, 0,
  2, 0, 0, 0, 0, 3, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
  18, 19, 20, 0, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, 1, 0, -7, -7, -7,
  2, 0, 0, 0, 0, 3, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
  18, 19, 20, 0, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, 1, 0, -7, 0, 0,
  2, 0, 0, 0, 0, 3, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
  18, 19, 20, 0, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, 0, 60, 61, 62, 63,
  64, 65, 0, 0, 0, 74, 60, 61, 62, 63,
  64, 65
};

static const yysigned_char yycheck[] = {
  1, 8, 8, 10, 0, 40, 37, 51, 8, 7,
  10, 41, 42, 43, 44, 45, 46, 48, 43, 44,
  45, 46, 52, 7, 42, 43, 44, 45, 46, 7,
  31, 4, 29, 34, 5, 36, 43, 44, 7, 7,
  47, 3, 49, 43, 44, 52, 7, 47, 87, 49,
  -1, -1, -1, 41, 42, 43, 44, 41, 42, 43,
  44, 45, 46, 41, 42, 43, 44, 45, 46, -1,
  71, 72, 60, 61, 62, 63, 64, 65, 66, -1,
  -1, 69, -1, 0, 1, 86, 3, 4, 5, 6,
  -1, -1, 93, 81, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, 28, -1, 30, 31, 32, 33, 34, 35, 36,
  37, 38, 39, 40, 0, 1, -1, -1, -1, -1,
  6, -1, -1, -1, -1, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  26, 27, 28, -1, 30, 31, 32, 33, 34, 35,
  36, 37, 38, 39, 40, 1, -1, 3, 4, 5,
  6, -1, -1, -1, -1, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  26, 27, 28, -1, 30, 31, 32, 33, 34, 35,
  36, 37, 38, 39, 40, 1, -1, 3, -1, -1,
  6, -1, -1, -1, -1, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  26, 27, 28, -1, 30, 31, 32, 33, 34, 35,
  36, 37, 38, 39, 40, -1, 41, 42, 43, 44,
  45, 46, -1, -1, -1, 50, 41, 42, 43, 44,
  45, 46
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] = {
  0, 1, 6, 11, 12, 13, 14, 15, 16, 17,
  18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
  28, 30, 31, 32, 33, 34, 35, 36, 37, 38,
  39, 40, 54, 55, 56, 63, 64, 65, 55, 8,
  10, 43, 44, 47, 49, 62, 8, 66, 67, 55,
  0, 55, 40, 55, 66, 62, 62, 62, 62, 7,
  41, 42, 43, 44, 45, 46, 51, 7, 66, 4,
  57, 58, 29, 7, 50, 62, 62, 62, 62, 62,
  62, 52, 62, 62, 5, 59, 60, 55, 55, 62,
  7, 7, 3, 61, 55, 57, 52, 7, 55
};

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
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
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
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
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
#  include <stdio.h>		/* INFRINGES ON USER NAME SPACE */
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
| TOP (cinluded).                                                   |
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
  for ( /* Nothing. */ ; bottom <= top; ++bottom)
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
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
	     yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
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

#if YYMAXDEPTH == 0
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
yysymprint (FILE * yyoutput, int yytype, YYSTYPE * yyvaluep)
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
yydestruct (int yytype, YYSTYPE * yyvaluep)
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
int
yyparse (void *YYPARSE_PARAM)
# else
int
yyparse (YYPARSE_PARAM)
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
  short yyssa[YYINITDEPTH];
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
		    &yyvs1, yysize * sizeof (*yyvsp), &yystacksize);

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
	if (!yyptr)
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
  yyval = yyvsp[1 - yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
    case 2:
#line 99 "albumtheme.y"
      {
	yy_parsed_doc = yyvsp[0].list;

	if (yy_parsed_doc == NULL)
	  YYABORT;
	else
	  YYACCEPT;
	;
      }
      break;

    case 3:
#line 109 "albumtheme.y"
      {
	yyval.list =
	  g_list_prepend (yyvsp[0].list, gth_tag_new_html (yyvsp[-1].text));
	g_free (yyvsp[-1].text);
	;
      }
      break;

    case 4:
#line 114 "albumtheme.y"
      {
	yyval.list = g_list_prepend (yyvsp[0].list, yyvsp[-1].tag);
	;
      }
      break;

    case 5:
#line 118 "albumtheme.y"
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
      }
      break;

    case 6:
#line 131 "albumtheme.y"
      {
	GthTag *tag = gth_tag_new_html (yyvsp[-2].text);
	GList *child_doc = g_list_append (NULL, tag);
	gth_tag_add_document (yyvsp[-3].tag, child_doc);
	yyval.list = g_list_prepend (yyvsp[0].list, yyvsp[-3].tag);
	g_free (yyvsp[-2].text);
	;
      }
      break;

    case 7:
#line 139 "albumtheme.y"
      {
	yyval.list = NULL;
	;
      }
      break;

    case 8:
#line 143 "albumtheme.y"
      {
	if (yyvsp[0].list != NULL)
	  gth_parsed_doc_free (yyvsp[0].list);
	yyval.list = NULL;
	;
      }
      break;

    case 9:
#line 150 "albumtheme.y"
      {
	yyval.cond = gth_condition_new (yyvsp[-1].expr);
	;
      }
      break;

    case 10:
#line 155 "albumtheme.y"
      {
	gth_condition_add_document (yyvsp[-2].cond, yyvsp[-1].list);
	yyval.list = g_list_prepend (yyvsp[0].list, yyvsp[-2].cond);
	;
      }
      break;

    case 11:
#line 160 "albumtheme.y"
      {
	yyval.list = NULL;
	;
      }
      break;

    case 12:
#line 165 "albumtheme.y"
      {
	yyval.cond = gth_condition_new (yyvsp[-1].expr);
	;
      }
      break;

    case 13:
#line 170 "albumtheme.y"
      {
	GthExpr *else_expr;
	GthCondition *cond;

	else_expr = gth_expr_new ();
	gth_expr_push_constant (else_expr, 1);
	cond = gth_condition_new (else_expr);
	gth_condition_add_document (cond, yyvsp[0].list);

	yyval.cond = cond;
	;
      }
      break;

    case 14:
#line 182 "albumtheme.y"
      {
	yyval.cond = NULL;
	;
      }
      break;

    case 17:
#line 193 "albumtheme.y"
      {
	yyval.expr = yyvsp[-1].expr;
	;
      }
      break;

    case 18:
#line 197 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, yyvsp[-2].expr);
	gth_expr_push_expr (e, yyvsp[0].expr);
	gth_expr_push_op (e, yyvsp[-1].ivalue);

	gth_expr_unref (yyvsp[-2].expr);
	gth_expr_unref (yyvsp[0].expr);

	yyval.expr = e;
	;
      }
      break;

    case 19:
#line 210 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, yyvsp[-2].expr);
	gth_expr_push_expr (e, yyvsp[0].expr);
	gth_expr_push_op (e, GTH_OP_ADD);

	gth_expr_unref (yyvsp[-2].expr);
	gth_expr_unref (yyvsp[0].expr);

	yyval.expr = e;
	;
      }
      break;

    case 20:
#line 223 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, yyvsp[-2].expr);
	gth_expr_push_expr (e, yyvsp[0].expr);
	gth_expr_push_op (e, GTH_OP_SUB);

	gth_expr_unref (yyvsp[-2].expr);
	gth_expr_unref (yyvsp[0].expr);

	yyval.expr = e;
	;
      }
      break;

    case 21:
#line 236 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, yyvsp[-2].expr);
	gth_expr_push_expr (e, yyvsp[0].expr);
	gth_expr_push_op (e, GTH_OP_MUL);

	gth_expr_unref (yyvsp[-2].expr);
	gth_expr_unref (yyvsp[0].expr);

	yyval.expr = e;
	;
      }
      break;

    case 22:
#line 249 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, yyvsp[-2].expr);
	gth_expr_push_expr (e, yyvsp[0].expr);
	gth_expr_push_op (e, GTH_OP_DIV);

	gth_expr_unref (yyvsp[-2].expr);
	gth_expr_unref (yyvsp[0].expr);

	yyval.expr = e;
	;
      }
      break;

    case 23:
#line 262 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, yyvsp[-2].expr);
	gth_expr_push_expr (e, yyvsp[0].expr);
	gth_expr_push_op (e, yyvsp[-1].ivalue);

	gth_expr_unref (yyvsp[-2].expr);
	gth_expr_unref (yyvsp[0].expr);

	yyval.expr = e;
	;
      }
      break;

    case 24:
#line 275 "albumtheme.y"
      {
	yyval.expr = yyvsp[0].expr;
	;
      }
      break;

    case 25:
#line 279 "albumtheme.y"
      {
	gth_expr_push_op (yyvsp[0].expr, GTH_OP_NEG);
	yyval.expr = yyvsp[0].expr;
	;
      }
      break;

    case 26:
#line 284 "albumtheme.y"
      {
	gth_expr_push_op (yyvsp[0].expr, GTH_OP_NOT);
	yyval.expr = yyvsp[0].expr;
	;
      }
      break;

    case 27:
#line 289 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	gth_expr_push_var (e, yyvsp[0].text);
	g_free (yyvsp[0].text);
	yyval.expr = e;
	;
      }
      break;

    case 28:
#line 296 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	gth_expr_push_constant (e, yyvsp[0].ivalue);
	yyval.expr = e;
	;
      }
      break;

    case 29:
#line 303 "albumtheme.y"
      {
	yyval.tag = gth_tag_new (yyvsp[-2].ivalue, yyvsp[-1].list);
	;
      }
      break;

    case 30:
#line 308 "albumtheme.y"
      {
	yyval.tag = gth_tag_new (yyvsp[-2].ivalue, yyvsp[-1].list);
	;
      }
      break;

    case 31:
#line 313 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 32:
#line 314 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 33:
#line 315 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 34:
#line 316 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 35:
#line 317 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 36:
#line 318 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 37:
#line 319 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 38:
#line 320 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 39:
#line 321 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 40:
#line 322 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 41:
#line 323 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 42:
#line 324 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 43:
#line 325 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 44:
#line 326 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 45:
#line 327 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 46:
#line 328 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 47:
#line 329 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 48:
#line 330 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 49:
#line 331 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 50:
#line 332 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 51:
#line 333 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 52:
#line 334 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 53:
#line 335 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 54:
#line 336 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 55:
#line 337 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 56:
#line 338 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 57:
#line 339 "albumtheme.y"
      {
	yyval.ivalue = yyvsp[0].ivalue;;
      }
      break;

    case 58:
#line 342 "albumtheme.y"
      {
	yyval.list = g_list_prepend (yyvsp[0].list, yyvsp[-1].var);
	;
      }
      break;

    case 59:
#line 346 "albumtheme.y"
      {
	yyval.list = NULL;
	;
      }
      break;

    case 60:
#line 351 "albumtheme.y"
      {
	yyval.var = gth_var_new_expression (yyvsp[-2].text, yyvsp[0].expr);
	g_free (yyvsp[-2].text);
	;
      }
      break;

    case 61:
#line 356 "albumtheme.y"
      {
	yyval.var = gth_var_new_expression (yyvsp[-4].text, yyvsp[-1].expr);
	g_free (yyvsp[-4].text);
	;
      }
      break;

    case 62:
#line 361 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	gth_expr_push_constant (e, 1);
	yyval.var = gth_var_new_expression (yyvsp[0].text, e);
	g_free (yyvsp[0].text);
	;
      }
      break;


    }

/* Line 999 of yacc.c.  */
#line 1673 "albumtheme.c"

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
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
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

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
	{
	  /* Pop the error token.  */
	  YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
	}

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;		/* Each real token shifted decrements this.  */

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
      yyvsp--;
      yystate = *--yyssp;

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


#line 369 "albumtheme.y"


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
