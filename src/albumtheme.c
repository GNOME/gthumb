/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

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
  PLACE = 278,
  DATE_TIME = 279,
  PAGE_LINK = 280,
  PAGE_IDX = 281,
  PAGES = 282,
  TABLE = 283,
  THUMBS = 284,
  DATE = 285,
  TEXT = 286,
  TEXT_END = 287,
  EXIF_EXPOSURE_TIME = 288,
  EXIF_EXPOSURE_MODE = 289,
  EXIF_FLASH = 290,
  EXIF_SHUTTER_SPEED = 291,
  EXIF_APERTURE_VALUE = 292,
  EXIF_FOCAL_LENGTH = 293,
  EXIF_DATE_TIME = 294,
  EXIF_CAMERA_MODEL = 295,
  SET_VAR = 296,
  EVAL = 297,
  HTML = 298,
  BOOL_OP = 299,
  COMPARE = 300,
  UNARY_OP = 301
};
#endif
/* Tokens.  */
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
#define PLACE 278
#define DATE_TIME 279
#define PAGE_LINK 280
#define PAGE_IDX 281
#define PAGES 282
#define TABLE 283
#define THUMBS 284
#define DATE 285
#define TEXT 286
#define TEXT_END 287
#define EXIF_EXPOSURE_TIME 288
#define EXIF_EXPOSURE_MODE 289
#define EXIF_FLASH 290
#define EXIF_SHUTTER_SPEED 291
#define EXIF_APERTURE_VALUE 292
#define EXIF_FOCAL_LENGTH 293
#define EXIF_DATE_TIME 294
#define EXIF_CAMERA_MODEL 295
#define SET_VAR 296
#define EVAL 297
#define HTML 298
#define BOOL_OP 299
#define COMPARE 300
#define UNARY_OP 301




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

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 35 "albumtheme.y"
{
  char *text;
  int ivalue;
  GString *string;
  GthVar *var;
  GthTag *tag;
  GthExpr *expr;
  GList *list;
  GthCondition *cond;
}
/* Line 187 of yacc.c.  */
#line 233 "albumtheme.c"
YYSTYPE;
# define yystype YYSTYPE	/* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 246 "albumtheme.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h>		/* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h>		/* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e)		/* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
     int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h>		/* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h>		/* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h>	/* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032	/* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h>		/* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T);	/* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *);		/* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
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
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  54
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   296

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  56
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  19
/* YYNRULES -- Number of rules.  */
#define YYNRULES  74
/* YYNRULES -- Number of states.  */
#define YYNSTATES  119

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   301

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] = {
  0, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 50, 52, 2, 2, 2, 2, 2,
  53, 54, 48, 46, 2, 47, 2, 49, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 55, 2, 2, 2, 2, 2, 2, 2, 2,
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
  35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
  45, 51
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] = {
  0, 0, 3, 5, 8, 11, 18, 23, 24, 27,
  31, 37, 41, 42, 46, 52, 55, 56, 59, 62,
  66, 70, 74, 78, 82, 86, 90, 93, 96, 99,
  101, 103, 105, 109, 111, 113, 115, 118, 119, 123,
  127, 129, 131, 133, 135, 137, 139, 141, 143, 145,
  147, 149, 151, 153, 155, 157, 159, 161, 163, 165,
  167, 169, 171, 173, 175, 177, 179, 181, 183, 185,
  187, 190, 191, 195, 201
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] = {
  57, 0, -1, 58, -1, 43, 58, -1, 71, 58,
  -1, 59, 58, 60, 62, 64, 58, -1, 70, 43,
  32, 58, -1, -1, 1, 58, -1, 6, 65, 7,
  -1, 6, 52, 66, 52, 7, -1, 61, 58, 60,
  -1, -1, 4, 65, 7, -1, 4, 52, 66, 52,
  7, -1, 63, 58, -1, -1, 5, 7, -1, 3,
  7, -1, 53, 65, 54, -1, 65, 45, 65, -1,
  65, 46, 65, -1, 65, 47, 65, -1, 65, 48,
  65, -1, 65, 49, 65, -1, 65, 44, 65, -1,
  46, 65, -1, 47, 65, -1, 50, 65, -1, 8,
  -1, 10, -1, 65, -1, 67, 68, 69, -1, 8,
  -1, 8, -1, 10, -1, 68, 69, -1, -1, 31,
  73, 7, -1, 72, 73, 7, -1, 11, -1, 12,
  -1, 13, -1, 14, -1, 15, -1, 16, -1, 17,
  -1, 18, -1, 19, -1, 20, -1, 21, -1, 22,
  -1, 23, -1, 24, -1, 25, -1, 26, -1, 27,
  -1, 28, -1, 29, -1, 30, -1, 33, -1, 34,
  -1, 35, -1, 36, -1, 37, -1, 38, -1, 39,
  -1, 40, -1, 41, -1, 42, -1, 74, 73, -1,
  -1, 8, 55, 65, -1, 8, 55, 52, 66, 52,
  -1, 8, -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] = {
  0, 107, 107, 117, 122, 126, 139, 147, 151, 158,
  161, 166, 171, 176, 179, 184, 196, 201, 204, 207,
  211, 224, 237, 250, 263, 276, 289, 293, 298, 303,
  310, 316, 319, 333, 339, 345, 351, 359, 363, 368,
  373, 374, 375, 376, 377, 378, 379, 380, 381, 382,
  383, 384, 385, 386, 387, 388, 389, 390, 391, 392,
  393, 394, 395, 396, 397, 398, 399, 400, 401, 402,
  405, 409, 414, 419, 424
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] = {
  "$end", "error", "$undefined", "END", "ELSE_IF", "ELSE", "IF",
  "END_TAG", "NAME", "STRING", "NUMBER", "HEADER", "FOOTER", "LANGUAGE",
  "IMAGE", "IMAGE_LINK", "IMAGE_IDX", "IMAGE_DIM", "IMAGES", "FILENAME",
  "FILEPATH", "FILESIZE", "COMMENT", "PLACE", "DATE_TIME", "PAGE_LINK",
  "PAGE_IDX", "PAGES", "TABLE", "THUMBS", "DATE", "TEXT", "TEXT_END",
  "EXIF_EXPOSURE_TIME", "EXIF_EXPOSURE_MODE", "EXIF_FLASH",
  "EXIF_SHUTTER_SPEED", "EXIF_APERTURE_VALUE", "EXIF_FOCAL_LENGTH",
  "EXIF_DATE_TIME", "EXIF_CAMERA_MODEL", "SET_VAR", "EVAL", "HTML",
  "BOOL_OP", "COMPARE", "'+'", "'-'", "'*'", "'/'", "'!'", "UNARY_OP",
  "'\"'", "'('", "')'", "'='", "$accept", "all", "document", "gthumb_if",
  "opt_if_list", "gthumb_else_if", "opt_else", "gthumb_else", "gthumb_end",
  "expr", "quoted_expr", "constant1", "constant", "constant_list",
  "gthumb_text_tag", "gthumb_tag", "tag_name", "arg_list", "arg", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] = {
  0, 256, 257, 258, 259, 260, 261, 262, 263, 264,
  265, 266, 267, 268, 269, 270, 271, 272, 273, 274,
  275, 276, 277, 278, 279, 280, 281, 282, 283, 284,
  285, 286, 287, 288, 289, 290, 291, 292, 293, 294,
  295, 296, 297, 298, 299, 300, 43, 45, 42, 47,
  33, 301, 34, 40, 41, 61
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] = {
  0, 56, 57, 58, 58, 58, 58, 58, 58, 59,
  59, 60, 60, 61, 61, 62, 62, 63, 64, 65,
  65, 65, 65, 65, 65, 65, 65, 65, 65, 65,
  65, 66, 66, 67, 68, 68, 69, 69, 70, 71,
  72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
  72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
  72, 72, 72, 72, 72, 72, 72, 72, 72, 72,
  73, 73, 74, 74, 74
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] = {
  0, 2, 1, 2, 2, 6, 4, 0, 2, 3,
  5, 3, 0, 3, 5, 2, 0, 2, 2, 3,
  3, 3, 3, 3, 3, 3, 2, 2, 2, 1,
  1, 1, 3, 1, 1, 1, 2, 0, 3, 3,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 0, 3, 5, 1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] = {
  0, 0, 0, 40, 41, 42, 43, 44, 45, 46,
  47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
  57, 58, 59, 71, 60, 61, 62, 63, 64, 65,
  66, 67, 68, 69, 0, 0, 2, 0, 0, 0,
  71, 8, 29, 30, 0, 0, 0, 0, 0, 0,
  74, 0, 71, 3, 1, 12, 0, 4, 0, 26,
  27, 28, 29, 31, 0, 0, 0, 9, 0, 0,
  0, 0, 0, 0, 0, 38, 70, 0, 16, 0,
  0, 39, 0, 34, 35, 37, 19, 25, 20, 21,
  22, 23, 24, 0, 72, 0, 0, 0, 0, 0,
  12, 6, 10, 37, 32, 0, 0, 13, 17, 0,
  0, 15, 11, 36, 73, 0, 18, 5, 14
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] = {
  -1, 35, 36, 37, 78, 79, 98, 99, 110, 63,
  64, 65, 103, 104, 38, 39, 40, 51, 52
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -59
static const yytype_int16 yypact[] = {
  167, 123, 3, -59, -59, -59, -59, -59, -59, -59,
  -59, -59, -59, -59, -59, -59, -59, -59, -59, -59,
  -59, -59, -59, 9, -59, -59, -59, -59, -59, -59,
  -59, -59, -59, -59, 123, 1, -59, 210, -23, 123,
  9, -59, -59, -59, 57, 57, 57, 72, 57, 40,
  -21, 36, 9, -59, -59, 47, 27, -59, 53, -59,
  -59, -59, 2, -22, 10, 8, -40, -59, 57, 57,
  57, 57, 57, 57, 11, -59, -59, 44, 61, 210,
  123, -59, 69, -59, -59, 8, -59, -17, -7, -59,
  -59, -59, -59, 72, -22, 72, 68, 74, 80, 253,
  47, -59, -59, 8, -59, 41, 43, -59, -59, 85,
  123, -59, -59, -59, -59, 92, -59, -59, -59
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] = {
  -59, -59, -1, -59, 5, -59, -59, -59, -59, 0,
  -58, -59, 35, -2, -59, -59, -59, -37, -59
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -34
static const yytype_int8 yytable[] = {
  41, 54, 49, 58, 68, 69, 70, 71, 72, 73,
  -33, 42, -33, 43, 86, 76, 83, 50, 84, 42,
  56, 43, 68, 69, 70, 71, 72, 73, 69, 70,
  71, 72, 73, 53, 74, 105, 55, 106, 57, 70,
  71, 72, 73, 75, 59, 60, 61, 67, 66, 44,
  45, 77, 42, 46, 43, 47, 48, 44, 45, 80,
  81, 46, 82, 93, 48, 42, 97, 43, 87, 88,
  89, 90, 91, 92, 94, 107, 102, 96, 100, 101,
  62, 108, 43, 109, 68, 69, 70, 71, 72, 73,
  44, 45, 116, 114, 46, 115, 95, 48, 111, 118,
  85, 113, 0, 44, 45, 112, 0, 46, 0, 117,
  48, 0, 68, 69, 70, 71, 72, 73, 44, 45,
  0, 0, 46, -7, 1, 48, -7, -7, -7, 2,
  0, 0, 0, 0, 3, 4, 5, 6, 7, 8,
  9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 20, 21, 22, 23, 0, 24, 25, 26, 27,
  28, 29, 30, 31, 32, 33, 34, -7, 1, 0,
  0, 0, 0, 2, 0, 0, 0, 0, 3, 4,
  5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 0,
  24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
  34, 1, 0, -7, -7, -7, 2, 0, 0, 0,
  0, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
  22, 23, 0, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, 34, 1, 0, -7, 0, 0, 2,
  0, 0, 0, 0, 3, 4, 5, 6, 7, 8,
  9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 20, 21, 22, 23, 0, 24, 25, 26, 27,
  28, 29, 30, 31, 32, 33, 34
};

static const yytype_int8 yycheck[] = {
  1, 0, 2, 40, 44, 45, 46, 47, 48, 49,
  8, 8, 10, 10, 54, 52, 8, 8, 10, 8,
  43, 10, 44, 45, 46, 47, 48, 49, 45, 46,
  47, 48, 49, 34, 55, 93, 37, 95, 39, 46,
  47, 48, 49, 7, 44, 45, 46, 7, 48, 46,
  47, 4, 8, 50, 10, 52, 53, 46, 47, 32,
  7, 50, 52, 52, 53, 8, 5, 10, 68, 69,
  70, 71, 72, 73, 74, 7, 7, 77, 79, 80,
  8, 7, 10, 3, 44, 45, 46, 47, 48, 49,
  46, 47, 7, 52, 50, 52, 52, 53, 99, 7,
  65, 103, -1, 46, 47, 100, -1, 50, -1, 110,
  53, -1, 44, 45, 46, 47, 48, 49, 46, 47,
  -1, -1, 50, 0, 1, 53, 3, 4, 5, 6,
  -1, -1, -1, -1, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, -1, 33, 34, 35, 36,
  37, 38, 39, 40, 41, 42, 43, 0, 1, -1,
  -1, -1, -1, 6, -1, -1, -1, -1, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, 26, 27, 28, 29, 30, 31, -1,
  33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
  43, 1, -1, 3, 4, 5, 6, -1, -1, -1,
  -1, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
  30, 31, -1, 33, 34, 35, 36, 37, 38, 39,
  40, 41, 42, 43, 1, -1, 3, -1, -1, 6,
  -1, -1, -1, -1, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, 28, 29, 30, 31, -1, 33, 34, 35, 36,
  37, 38, 39, 40, 41, 42, 43
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] = {
  0, 1, 6, 11, 12, 13, 14, 15, 16, 17,
  18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 33, 34, 35, 36, 37, 38,
  39, 40, 41, 42, 43, 57, 58, 59, 70, 71,
  72, 58, 8, 10, 46, 47, 50, 52, 53, 65,
  8, 73, 74, 58, 0, 58, 43, 58, 73, 65,
  65, 65, 8, 65, 66, 67, 65, 7, 44, 45,
  46, 47, 48, 49, 55, 7, 73, 4, 60, 61,
  32, 7, 52, 8, 10, 68, 54, 65, 65, 65,
  65, 65, 65, 52, 65, 52, 65, 5, 62, 63,
  58, 58, 7, 68, 69, 66, 66, 7, 7, 3,
  64, 58, 60, 69, 52, 52, 7, 58, 7
};

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
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

 /*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
  static void
yy_symbol_value_print (FILE * yyoutput, int yytype,
		       YYSTYPE const *const yyvaluep)
#else
  static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
     FILE *yyoutput;
     int yytype;
     YYSTYPE const *const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
    default:
      break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE * yyoutput, int yytype, YYSTYPE const *const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
     FILE *yyoutput;
     int yytype;
     YYSTYPE const *const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 * bottom, yytype_int16 * top)
#else
static void
yy_stack_print (bottom, top)
     yytype_int16 *bottom;
     yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE * yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
     YYSTYPE *yyvsp;
     int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)]));
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
     const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes:;
    }

  if (!yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (!(YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum
      { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
         constructed on the fly.  */
      YY_ ("syntax error, unexpected %s");
      YY_ ("syntax error, unexpected %s, expecting %s");
      YY_ ("syntax error, unexpected %s, expecting %s or %s");
      YY_ ("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_ ("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_ (yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

 /*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
  static void
yydestruct (const char *yymsg, int yytype, YYSTYPE * yyvaluep)
#else
  static void
yydestruct (yymsg, yytype, yyvaluep)
     const char *yymsg;
     int yytype;
     YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

    default:
      break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
     void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()
#endif
#endif
{

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

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
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_ ("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp), &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (!yyptr)
	  goto yyexhaustedlab;
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

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
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
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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
#line 107 "albumtheme.y"
      {
	yy_parsed_doc = (yyvsp[(1) - (1)].list);

	if (yy_parsed_doc == NULL)
	  YYABORT;
	else
	  YYACCEPT;
	;
      }
      break;

    case 3:
#line 117 "albumtheme.y"
      {
	(yyval.list) =
	  g_list_prepend ((yyvsp[(2) - (2)].list),
			  gth_tag_new_html ((yyvsp[(1) - (2)].text)));
	g_free ((yyvsp[(1) - (2)].text));
	;
      }
      break;

    case 4:
#line 122 "albumtheme.y"
      {
	(yyval.list) =
	  g_list_prepend ((yyvsp[(2) - (2)].list), (yyvsp[(1) - (2)].tag));
	;
      }
      break;

    case 5:
#line 126 "albumtheme.y"
      {
	GList *cond_list;
	GthTag *tag;

	gth_condition_add_document ((yyvsp[(1) - (6)].cond),
				    (yyvsp[(2) - (6)].list));
	cond_list =
	  g_list_prepend ((yyvsp[(3) - (6)].list), (yyvsp[(1) - (6)].cond));
	if ((yyvsp[(4) - (6)].cond) != NULL)
	  cond_list = g_list_append (cond_list, (yyvsp[(4) - (6)].cond));

	tag = gth_tag_new_condition (cond_list);
	(yyval.list) = g_list_prepend ((yyvsp[(6) - (6)].list), tag);
	;
      }
      break;

    case 6:
#line 139 "albumtheme.y"
      {
	GthTag *tag = gth_tag_new_html ((yyvsp[(2) - (4)].text));
	GList *child_doc = g_list_append (NULL, tag);
	gth_tag_add_document ((yyvsp[(1) - (4)].tag), child_doc);
	(yyval.list) =
	  g_list_prepend ((yyvsp[(4) - (4)].list), (yyvsp[(1) - (4)].tag));
	g_free ((yyvsp[(2) - (4)].text));
	;
      }
      break;

    case 7:
#line 147 "albumtheme.y"
      {
	(yyval.list) = NULL;
	;
      }
      break;

    case 8:
#line 151 "albumtheme.y"
      {
	if ((yyvsp[(2) - (2)].list) != NULL)
	  gth_parsed_doc_free ((yyvsp[(2) - (2)].list));
	(yyval.list) = NULL;
	;
      }
      break;

    case 9:
#line 158 "albumtheme.y"
      {
	(yyval.cond) = gth_condition_new ((yyvsp[(2) - (3)].expr));
	;
      }
      break;

    case 10:
#line 161 "albumtheme.y"
      {
	(yyval.cond) = gth_condition_new ((yyvsp[(3) - (5)].expr));
	;
      }
      break;

    case 11:
#line 166 "albumtheme.y"
      {
	gth_condition_add_document ((yyvsp[(1) - (3)].cond),
				    (yyvsp[(2) - (3)].list));
	(yyval.list) =
	  g_list_prepend ((yyvsp[(3) - (3)].list), (yyvsp[(1) - (3)].cond));
	;
      }
      break;

    case 12:
#line 171 "albumtheme.y"
      {
	(yyval.list) = NULL;
	;
      }
      break;

    case 13:
#line 176 "albumtheme.y"
      {
	(yyval.cond) = gth_condition_new ((yyvsp[(2) - (3)].expr));
	;
      }
      break;

    case 14:
#line 179 "albumtheme.y"
      {
	(yyval.cond) = gth_condition_new ((yyvsp[(3) - (5)].expr));
	;
      }
      break;

    case 15:
#line 184 "albumtheme.y"
      {
	GthExpr *else_expr;
	GthCondition *cond;

	else_expr = gth_expr_new ();
	gth_expr_push_constant (else_expr, 1);
	cond = gth_condition_new (else_expr);
	gth_condition_add_document (cond, (yyvsp[(2) - (2)].list));

	(yyval.cond) = cond;
	;
      }
      break;

    case 16:
#line 196 "albumtheme.y"
      {
	(yyval.cond) = NULL;
	;
      }
      break;

    case 19:
#line 207 "albumtheme.y"
      {
	(yyval.expr) = (yyvsp[(2) - (3)].expr);
	;
      }
      break;

    case 20:
#line 211 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, (yyvsp[(1) - (3)].expr));
	gth_expr_push_expr (e, (yyvsp[(3) - (3)].expr));
	gth_expr_push_op (e, (yyvsp[(2) - (3)].ivalue));

	gth_expr_unref ((yyvsp[(1) - (3)].expr));
	gth_expr_unref ((yyvsp[(3) - (3)].expr));

	(yyval.expr) = e;
	;
      }
      break;

    case 21:
#line 224 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, (yyvsp[(1) - (3)].expr));
	gth_expr_push_expr (e, (yyvsp[(3) - (3)].expr));
	gth_expr_push_op (e, GTH_OP_ADD);

	gth_expr_unref ((yyvsp[(1) - (3)].expr));
	gth_expr_unref ((yyvsp[(3) - (3)].expr));

	(yyval.expr) = e;
	;
      }
      break;

    case 22:
#line 237 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, (yyvsp[(1) - (3)].expr));
	gth_expr_push_expr (e, (yyvsp[(3) - (3)].expr));
	gth_expr_push_op (e, GTH_OP_SUB);

	gth_expr_unref ((yyvsp[(1) - (3)].expr));
	gth_expr_unref ((yyvsp[(3) - (3)].expr));

	(yyval.expr) = e;
	;
      }
      break;

    case 23:
#line 250 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, (yyvsp[(1) - (3)].expr));
	gth_expr_push_expr (e, (yyvsp[(3) - (3)].expr));
	gth_expr_push_op (e, GTH_OP_MUL);

	gth_expr_unref ((yyvsp[(1) - (3)].expr));
	gth_expr_unref ((yyvsp[(3) - (3)].expr));

	(yyval.expr) = e;
	;
      }
      break;

    case 24:
#line 263 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, (yyvsp[(1) - (3)].expr));
	gth_expr_push_expr (e, (yyvsp[(3) - (3)].expr));
	gth_expr_push_op (e, GTH_OP_DIV);

	gth_expr_unref ((yyvsp[(1) - (3)].expr));
	gth_expr_unref ((yyvsp[(3) - (3)].expr));

	(yyval.expr) = e;
	;
      }
      break;

    case 25:
#line 276 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();

	gth_expr_push_expr (e, (yyvsp[(1) - (3)].expr));
	gth_expr_push_expr (e, (yyvsp[(3) - (3)].expr));
	gth_expr_push_op (e, (yyvsp[(2) - (3)].ivalue));

	gth_expr_unref ((yyvsp[(1) - (3)].expr));
	gth_expr_unref ((yyvsp[(3) - (3)].expr));

	(yyval.expr) = e;
	;
      }
      break;

    case 26:
#line 289 "albumtheme.y"
      {
	(yyval.expr) = (yyvsp[(2) - (2)].expr);
	;
      }
      break;

    case 27:
#line 293 "albumtheme.y"
      {
	gth_expr_push_op ((yyvsp[(2) - (2)].expr), GTH_OP_NEG);
	(yyval.expr) = (yyvsp[(2) - (2)].expr);
	;
      }
      break;

    case 28:
#line 298 "albumtheme.y"
      {
	gth_expr_push_op ((yyvsp[(2) - (2)].expr), GTH_OP_NOT);
	(yyval.expr) = (yyvsp[(2) - (2)].expr);
	;
      }
      break;

    case 29:
#line 303 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	gth_expr_push_var (e, (yyvsp[(1) - (1)].text));
	g_free ((yyvsp[(1) - (1)].text));
	(yyval.expr) = e;
	;
      }
      break;

    case 30:
#line 310 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	gth_expr_push_constant (e, (yyvsp[(1) - (1)].ivalue));
	(yyval.expr) = e;
	;
      }
      break;

    case 31:
#line 316 "albumtheme.y"
      {
	(yyval.expr) = (yyvsp[(1) - (1)].expr);
	;
      }
      break;

    case 32:
#line 319 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	g_string_append ((yyvsp[(1) - (3)].string),
			 (yyvsp[(2) - (3)].string)->str);
	g_string_free ((yyvsp[(2) - (3)].string), TRUE);
	if ((yyvsp[(3) - (3)].string) != NULL)
	  {
	    g_string_append ((yyvsp[(1) - (3)].string),
			     (yyvsp[(3) - (3)].string)->str);
	    g_string_free ((yyvsp[(3) - (3)].string), TRUE);
	  }
	gth_expr_push_var (e, (yyvsp[(1) - (3)].string)->str);
	g_string_free ((yyvsp[(1) - (3)].string), TRUE);
	(yyval.expr) = e;
	;
      }
      break;

    case 33:
#line 333 "albumtheme.y"
      {
	GString *s = g_string_new ((yyvsp[(1) - (1)].text));
	g_free ((yyvsp[(1) - (1)].text));
	(yyval.string) = s;
	;
      }
      break;

    case 34:
#line 339 "albumtheme.y"
      {
	GString *s = g_string_new ((yyvsp[(1) - (1)].text));
	g_string_prepend_c (s, ' ');
	g_free ((yyvsp[(1) - (1)].text));
	(yyval.string) = s;
	;
      }
      break;

    case 35:
#line 345 "albumtheme.y"
      {
	GString *s = g_string_new ("");
	g_string_sprintf (s, " %i", (yyvsp[(1) - (1)].ivalue));
	(yyval.string) = s;
	;
      }
      break;

    case 36:
#line 351 "albumtheme.y"
      {
	if ((yyvsp[(2) - (2)].string) != NULL)
	  {
	    g_string_append ((yyvsp[(1) - (2)].string),
			     (yyvsp[(2) - (2)].string)->str);
	    g_string_free ((yyvsp[(2) - (2)].string), TRUE);
	  }
	(yyval.string) = (yyvsp[(1) - (2)].string);
	;
      }
      break;

    case 37:
#line 359 "albumtheme.y"
      {
	(yyval.string) = NULL;
	;
      }
      break;

    case 38:
#line 363 "albumtheme.y"
      {
	(yyval.tag) =
	  gth_tag_new ((yyvsp[(1) - (3)].ivalue), (yyvsp[(2) - (3)].list));
	;
      }
      break;

    case 39:
#line 368 "albumtheme.y"
      {
	(yyval.tag) =
	  gth_tag_new ((yyvsp[(1) - (3)].ivalue), (yyvsp[(2) - (3)].list));
	;
      }
      break;

    case 40:
#line 373 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 41:
#line 374 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 42:
#line 375 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 43:
#line 376 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 44:
#line 377 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 45:
#line 378 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 46:
#line 379 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 47:
#line 380 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 48:
#line 381 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 49:
#line 382 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 50:
#line 383 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 51:
#line 384 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 52:
#line 385 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 53:
#line 386 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 54:
#line 387 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 55:
#line 388 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 56:
#line 389 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 57:
#line 390 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 58:
#line 391 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 59:
#line 392 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 60:
#line 393 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 61:
#line 394 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 62:
#line 395 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 63:
#line 396 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 64:
#line 397 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 65:
#line 398 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 66:
#line 399 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 67:
#line 400 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 68:
#line 401 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 69:
#line 402 "albumtheme.y"
      {
	(yyval.ivalue) = (yyvsp[(1) - (1)].ivalue);;
      }
      break;

    case 70:
#line 405 "albumtheme.y"
      {
	(yyval.list) =
	  g_list_prepend ((yyvsp[(2) - (2)].list), (yyvsp[(1) - (2)].var));
	;
      }
      break;

    case 71:
#line 409 "albumtheme.y"
      {
	(yyval.list) = NULL;
	;
      }
      break;

    case 72:
#line 414 "albumtheme.y"
      {
	(yyval.var) =
	  gth_var_new_expression ((yyvsp[(1) - (3)].text),
				  (yyvsp[(3) - (3)].expr));
	g_free ((yyvsp[(1) - (3)].text));
	;
      }
      break;

    case 73:
#line 419 "albumtheme.y"
      {
	(yyval.var) =
	  gth_var_new_expression ((yyvsp[(1) - (5)].text),
				  (yyvsp[(4) - (5)].expr));
	g_free ((yyvsp[(1) - (5)].text));
	;
      }
      break;

    case 74:
#line 424 "albumtheme.y"
      {
	GthExpr *e = gth_expr_new ();
	gth_expr_push_constant (e, 1);
	(yyval.var) = gth_var_new_expression ((yyvsp[(1) - (1)].text), e);
	g_free ((yyvsp[(1) - (1)].text));
	;
      }
      break;


/* Line 1267 of yacc.c.  */
#line 2142 "albumtheme.c"
    default:
      break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
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
#if ! YYERROR_VERBOSE
      yyerror (YY_ ("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (!(yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_ ("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if ( /*CONSTCOND*/ 0)
    goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
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


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_ ("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
    yydestruct ("Cleanup: discarding lookahead", yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping", yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 434 "albumtheme.y"


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
