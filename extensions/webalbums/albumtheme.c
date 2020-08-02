/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.5.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         gth_albumtheme_yyparse
#define yylex           gth_albumtheme_yylex
#define yyerror         gth_albumtheme_yyerror
#define yydebug         gth_albumtheme_yydebug
#define yynerrs         gth_albumtheme_yynerrs
#define yylval          gth_albumtheme_yylval
#define yychar          gth_albumtheme_yychar

/* First part of user prologue.  */
#line 1 "albumtheme.y"

/*
 *  Pix
 *
 *  Copyright (C) 2003, 2010 Free Software Foundation, Inc.
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
#include <gio/gio.h>
#include "albumtheme-private.h"

int   gth_albumtheme_yylex   ();
void  gth_albumtheme_yyerror (const char *fmt, ...);
int   gth_albumtheme_yywrap  (void);

#define YY_NO_UNPUT
#define YYERROR_VERBOSE 1


#line 113 "albumtheme.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int gth_albumtheme_yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    IF = 258,
    ELSE = 259,
    ELSE_IF = 260,
    END = 261,
    SET_VAR = 262,
    PRINT = 263,
    END_TAG = 264,
    VARIABLE = 265,
    ATTRIBUTE_NAME = 266,
    FUNCTION_NAME = 267,
    QUOTED_STRING = 268,
    FOR_EACH = 269,
    NUMBER = 270,
    HTML = 271,
    BOOL_OP = 272,
    COMPARE = 273,
    RANGE = 274,
    IN = 275,
    UNARY_OP = 276
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 37 "albumtheme.y"

	char         *text;
	int           ivalue;
	GthAttribute *attribute;
	GthTag       *tag;
	GthExpr      *expr;
	GList        *list;
	GthCondition *cond;
	GthLoop      *loop;

#line 195 "albumtheme.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE gth_albumtheme_yylval;

int gth_albumtheme_yyparse (void);





#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))

/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
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
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  29
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   239

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  33
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  16
/* YYNRULES -- Number of rules.  */
#define YYNRULES  46
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  104

#define YYUNDEFTOK  2
#define YYMAXUTOK   276


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    25,    28,     2,     2,     2,     2,    30,
      31,    32,    23,    21,    26,    22,     2,    24,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    29,     2,     2,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    27
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    77,    77,    86,    91,    95,    99,   107,   120,   124,
     131,   144,   154,   158,   163,   168,   173,   177,   182,   194,
     199,   202,   205,   210,   258,   268,   272,   277,   282,   288,
     296,   300,   304,   309,   313,   326,   339,   352,   365,   378,
     391,   395,   400,   405,   422,   429,   436
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IF", "ELSE", "ELSE_IF", "END",
  "SET_VAR", "PRINT", "END_TAG", "VARIABLE", "ATTRIBUTE_NAME",
  "FUNCTION_NAME", "QUOTED_STRING", "FOR_EACH", "NUMBER", "HTML",
  "BOOL_OP", "COMPARE", "RANGE", "IN", "'+'", "'-'", "'*'", "'/'", "'!'",
  "','", "UNARY_OP", "'\"'", "'='", "'\\''", "'('", "')'", "$accept",
  "all", "document", "tag_loop", "tag_if", "opt_tag_else_if",
  "tag_else_if", "opt_tag_else", "tag_else", "tag_end", "tag_command",
  "tag_print", "attribute_list", "attribute", "expr_list", "expr", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,    43,    45,    42,    47,    33,    44,   276,    34,    61,
      39,    40,    41
};
# endif

#define YYPACT_NINF (-28)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-9)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     112,   102,     5,    17,    -9,    19,   102,    31,   -28,   138,
     126,   102,   102,   -28,    10,   -28,    70,    70,    70,    70,
      38,    70,   139,     3,    43,    17,    39,    -8,   -28,   -28,
      47,    52,   -28,   -28,    70,   -28,   -28,   -28,   181,    32,
     147,   -28,    70,    70,    70,    70,    70,    70,   -24,   -28,
     -28,    56,    57,   205,   -28,    70,    58,   102,    53,    68,
     126,    41,    67,   -28,   -28,    16,     1,   -28,   -28,   -28,
     -28,    70,    64,   -28,   -28,    70,   215,   -28,   -28,    70,
     163,    77,    47,   138,    52,   -28,   -28,   189,    59,   -28,
      70,   197,   -28,   -28,   102,   -28,   -28,   -28,   -28,   173,
      78,   -28,   -28,   -28
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     0,    26,     0,     0,     0,     0,     2,     0,
       0,     0,     0,     9,    44,    46,     0,     0,     0,     0,
       0,     0,     0,    29,     0,    26,    26,     0,     3,     1,
       0,    15,     4,     5,    32,    40,    41,    42,     0,     0,
       0,    12,     0,     0,     0,     0,     0,     0,     0,    22,
      25,     0,     0,    31,    10,     0,     0,     0,     0,    19,
       0,     0,     0,    45,    33,    39,    34,    35,    36,    37,
      38,     0,     0,    24,    23,    32,     0,    21,     6,     0,
       0,     0,     0,     0,    15,    43,    13,     0,     0,    30,
       0,     0,    16,    20,     0,    18,    14,    27,    28,     0,
       0,     7,    11,    17
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -28,   -28,    -1,   -28,   -28,     4,   -28,   -28,   -28,    12,
     -28,   -28,   -12,   -28,   -27,     0
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     7,     8,     9,    10,    59,    60,    82,    83,    57,
      11,    12,    24,    25,    52,    53
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      13,    54,    22,    26,    71,    28,    72,    61,    30,    31,
      32,    33,    55,    50,    51,    14,    35,    36,    37,    38,
      15,    40,    44,    45,    46,    47,    16,    17,    23,    27,
      18,    29,    48,    19,    43,    20,    21,    44,    45,    46,
      47,    34,    65,    66,    67,    68,    69,    70,    89,    14,
      23,    39,    49,    56,    15,    76,    78,    58,    80,    84,
      16,    17,    63,    14,    18,    73,    74,    77,    15,    20,
      21,    87,    81,    85,    16,    17,    86,    88,    18,    91,
      14,    79,    95,    20,    21,    15,    93,   103,    96,    98,
      99,    16,    17,   101,    94,    18,     0,     0,     0,     0,
      20,    21,    -8,     1,     0,     2,    -8,    -8,    -8,     3,
       4,     0,    -8,     1,     0,     2,     5,     0,     6,     3,
       4,     0,     0,     0,     0,     0,     5,     1,     6,     2,
      -8,    -8,    -8,     3,     4,     0,     0,     0,     0,     1,
       5,     2,     6,     0,    -8,     3,     4,     0,    41,     0,
       0,     0,     5,     0,     6,     0,    42,    43,     0,     0,
      44,    45,    46,    47,    42,    43,     0,     0,    44,    45,
      46,    47,    92,     0,     0,     0,     0,     0,     0,    64,
      42,    43,   102,     0,    44,    45,    46,    47,     0,     0,
      42,    43,     0,     0,    44,    45,    46,    47,    42,    43,
       0,     0,    44,    45,    46,    47,    42,    43,     0,    62,
      44,    45,    46,    47,    42,    43,     0,    97,    44,    45,
      46,    47,    42,    43,     0,   100,    44,    45,    46,    47,
       0,    75,    42,    43,    90,     0,    44,    45,    46,    47
};

static const yytype_int8 yycheck[] =
{
       1,     9,     2,    12,    28,     6,    30,    34,     9,    10,
      11,    12,    20,    25,    26,    10,    16,    17,    18,    19,
      15,    21,    21,    22,    23,    24,    21,    22,    11,    10,
      25,     0,    29,    28,    18,    30,    31,    21,    22,    23,
      24,    31,    42,    43,    44,    45,    46,    47,    75,    10,
      11,    13,     9,     6,    15,    55,    57,     5,    58,    60,
      21,    22,    30,    10,    25,     9,     9,     9,    15,    30,
      31,    71,     4,    32,    21,    22,     9,    13,    25,    79,
      10,    28,    83,    30,    31,    15,     9,     9,    84,    30,
      90,    21,    22,    94,    82,    25,    -1,    -1,    -1,    -1,
      30,    31,     0,     1,    -1,     3,     4,     5,     6,     7,
       8,    -1,     0,     1,    -1,     3,    14,    -1,    16,     7,
       8,    -1,    -1,    -1,    -1,    -1,    14,     1,    16,     3,
       4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,     1,
      14,     3,    16,    -1,     6,     7,     8,    -1,     9,    -1,
      -1,    -1,    14,    -1,    16,    -1,    17,    18,    -1,    -1,
      21,    22,    23,    24,    17,    18,    -1,    -1,    21,    22,
      23,    24,     9,    -1,    -1,    -1,    -1,    -1,    -1,    32,
      17,    18,     9,    -1,    21,    22,    23,    24,    -1,    -1,
      17,    18,    -1,    -1,    21,    22,    23,    24,    17,    18,
      -1,    -1,    21,    22,    23,    24,    17,    18,    -1,    28,
      21,    22,    23,    24,    17,    18,    -1,    28,    21,    22,
      23,    24,    17,    18,    -1,    28,    21,    22,    23,    24,
      -1,    26,    17,    18,    19,    -1,    21,    22,    23,    24
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     1,     3,     7,     8,    14,    16,    34,    35,    36,
      37,    43,    44,    35,    10,    15,    21,    22,    25,    28,
      30,    31,    48,    11,    45,    46,    12,    10,    35,     0,
      35,    35,    35,    35,    31,    48,    48,    48,    48,    13,
      48,     9,    17,    18,    21,    22,    23,    24,    29,     9,
      45,    45,    47,    48,     9,    20,     6,    42,     5,    38,
      39,    47,    28,    30,    32,    48,    48,    48,    48,    48,
      48,    28,    30,     9,     9,    26,    48,     9,    35,    28,
      48,     4,    40,    41,    35,    32,     9,    48,    13,    47,
      19,    48,     9,     9,    42,    35,    38,    28,    30,    48,
      28,    35,     9,     9
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    33,    34,    35,    35,    35,    35,    35,    35,    35,
      36,    36,    37,    37,    38,    38,    39,    39,    40,    40,
      41,    42,    43,    44,    44,    45,    45,    46,    46,    46,
      47,    47,    47,    48,    48,    48,    48,    48,    48,    48,
      48,    48,    48,    48,    48,    48,    48
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     2,     2,     4,     6,     0,     2,
       3,     7,     3,     5,     3,     0,     3,     5,     2,     0,
       2,     2,     3,     4,     4,     2,     0,     5,     5,     1,
       3,     1,     0,     3,     3,     3,     3,     3,     3,     3,
       2,     2,     2,     4,     1,     3,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[+yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

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
#ifndef YYINITDEPTH
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
#   define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#  else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
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
            else
              goto append;

          append:
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
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                yy_state_t *yyssp, int yytoken)
{
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Actual size of YYARG. */
  int yycount = 0;
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[+*yyssp];
      YYPTRDIFF_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
      yysize = yysize0;
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYPTRDIFF_T yysize1
                    = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    /* Don't count the "%s"s in the final size, but reserve room for
       the terminator.  */
    YYPTRDIFF_T yysize1 = yysize + (yystrlen (yyformat) - 2 * yycount) + 1;
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss;
    yy_state_t *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYPTRDIFF_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
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
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2:
#line 77 "albumtheme.y"
                           {
			yy_parsed_doc = (yyvsp[0].list);
			if (yy_parsed_doc == NULL)
				YYABORT;
			else
				YYACCEPT;
		}
#line 1464 "albumtheme.c"
    break;

  case 3:
#line 86 "albumtheme.y"
                                {
			(yyval.list) = g_list_prepend ((yyvsp[0].list), gth_tag_new_html ((yyvsp[-1].text)));
			g_free ((yyvsp[-1].text));
		}
#line 1473 "albumtheme.c"
    break;

  case 4:
#line 91 "albumtheme.y"
                                       {
			(yyval.list) = g_list_prepend ((yyvsp[0].list), (yyvsp[-1].tag));
		}
#line 1481 "albumtheme.c"
    break;

  case 5:
#line 95 "albumtheme.y"
                                     {
			(yyval.list) = g_list_prepend ((yyvsp[0].list), (yyvsp[-1].tag));
		}
#line 1489 "albumtheme.c"
    break;

  case 6:
#line 99 "albumtheme.y"
                                                     {
			GthTag *tag;

			gth_loop_add_document ((yyvsp[-3].loop), (yyvsp[-2].list));
			tag = gth_tag_new_loop ((yyvsp[-3].loop));
			(yyval.list) = g_list_prepend ((yyvsp[0].list), tag);
		}
#line 1501 "albumtheme.c"
    break;

  case 7:
#line 107 "albumtheme.y"
                                                                                {
			GList  *cond_list;
			GthTag *tag;

			gth_condition_add_document ((yyvsp[-5].cond), (yyvsp[-4].list));
			cond_list = g_list_prepend ((yyvsp[-3].list), (yyvsp[-5].cond));
			if ((yyvsp[-2].cond) != NULL)
				cond_list = g_list_append (cond_list, (yyvsp[-2].cond));

			tag = gth_tag_new_condition (cond_list);
			(yyval.list) = g_list_prepend ((yyvsp[0].list), tag);
		}
#line 1518 "albumtheme.c"
    break;

  case 8:
#line 120 "albumtheme.y"
                              {
			(yyval.list) = NULL;
		}
#line 1526 "albumtheme.c"
    break;

  case 9:
#line 124 "albumtheme.y"
                                 {
			/*if ($2 != NULL)
				gth_parsed_doc_free ($2);*/
			(yyval.list) = NULL;
		}
#line 1536 "albumtheme.c"
    break;

  case 10:
#line 131 "albumtheme.y"
                                            {
			if (g_str_equal ((yyvsp[-1].text), "thumbnail_caption")) {
				(yyval.loop) = gth_loop_new (GTH_TAG_FOR_EACH_THUMBNAIL_CAPTION);
			}
			else if (g_str_equal ((yyvsp[-1].text), "image_caption")) {
				(yyval.loop) = gth_loop_new (GTH_TAG_FOR_EACH_IMAGE_CAPTION);
			}
			else {
				yyerror ("Wrong iterator: '%s', expected 'thumbnail_caption' or 'image_caption'", (yyvsp[-1].text));
				YYERROR;
			}
		}
#line 1553 "albumtheme.c"
    break;

  case 11:
#line 144 "albumtheme.y"
                                                               {
			(yyval.loop) = gth_range_loop_new ();
			gth_range_loop_set_range (GTH_RANGE_LOOP ((yyval.loop)), (yyvsp[-5].text), (yyvsp[-3].expr), (yyvsp[-1].expr));

			g_free ((yyvsp[-5].text));
			gth_expr_unref ((yyvsp[-3].expr));
			gth_expr_unref ((yyvsp[-1].expr));
		}
#line 1566 "albumtheme.c"
    break;

  case 12:
#line 154 "albumtheme.y"
                                  {
			(yyval.cond) = gth_condition_new ((yyvsp[-1].expr));
		}
#line 1574 "albumtheme.c"
    break;

  case 13:
#line 158 "albumtheme.y"
                                          {
			(yyval.cond) = gth_condition_new ((yyvsp[-2].expr));
		}
#line 1582 "albumtheme.c"
    break;

  case 14:
#line 163 "albumtheme.y"
                                                       {
			gth_condition_add_document ((yyvsp[-2].cond), (yyvsp[-1].list));
			(yyval.list) = g_list_prepend ((yyvsp[0].list), (yyvsp[-2].cond));
		}
#line 1591 "albumtheme.c"
    break;

  case 15:
#line 168 "albumtheme.y"
                              {
			(yyval.list) = NULL;
		}
#line 1599 "albumtheme.c"
    break;

  case 16:
#line 173 "albumtheme.y"
                                       {
			(yyval.cond) = gth_condition_new ((yyvsp[-1].expr));
		}
#line 1607 "albumtheme.c"
    break;

  case 17:
#line 177 "albumtheme.y"
                                               {
			(yyval.cond) = gth_condition_new ((yyvsp[-2].expr));
		}
#line 1615 "albumtheme.c"
    break;

  case 18:
#line 182 "albumtheme.y"
                                    {
			GthExpr      *else_expr;
			GthCondition *cond;

			else_expr = gth_expr_new ();
			gth_expr_push_integer (else_expr, 1);
			cond = gth_condition_new (else_expr);
			gth_condition_add_document (cond, (yyvsp[0].list));

			(yyval.cond) = cond;
		}
#line 1631 "albumtheme.c"
    break;

  case 19:
#line 194 "albumtheme.y"
                              {
			(yyval.cond) = NULL;
		}
#line 1639 "albumtheme.c"
    break;

  case 22:
#line 205 "albumtheme.y"
                                                 {
			(yyval.tag) = gth_tag_new (GTH_TAG_SET_VAR, (yyvsp[-1].list));
		}
#line 1647 "albumtheme.c"
    break;

  case 23:
#line 210 "albumtheme.y"
                                                        {
			if (gth_tag_get_type_from_name ((yyvsp[-2].text)) == GTH_TAG_EVAL) {
				GthExpr *e;
				GList   *arg_list;

				if ((yyvsp[-1].list) == NULL) {
					yyerror ("Missing argument for function 'eval', expected expression");
					YYERROR;
				}

				e = (yyvsp[-1].list)->data;
				arg_list = g_list_append (NULL, gth_attribute_new_expression ("expr", e));
				(yyval.tag) = gth_tag_new (GTH_TAG_EVAL, arg_list);

				gth_expr_list_unref ((yyvsp[-1].list));
			}
			else if (gth_tag_get_type_from_name ((yyvsp[-2].text)) == GTH_TAG_TRANSLATE) {
				GList *arg_list = NULL;
				GList *scan;

				for (scan = (yyvsp[-1].list); scan; scan = scan->next) {
					GthExpr *e = scan->data;

					if (scan == (yyvsp[-1].list)) {
						GthCell *cell;

						cell = gth_expr_get (e);
						if (cell->type != GTH_CELL_TYPE_STRING) {
							yyerror ("Wrong argument type: %d, expected string", cell->type);
							YYERROR;
						}
						arg_list = g_list_append (arg_list, gth_attribute_new_string ("text", cell->value.string->str));

						continue;
					}

					arg_list = g_list_append (arg_list, gth_attribute_new_expression ("expr", e));
				}
				(yyval.tag) = gth_tag_new (GTH_TAG_TRANSLATE, arg_list);

				gth_expr_list_unref ((yyvsp[-1].list));
			}
			else {
				yyerror ("Wrong function: '%s', expected 'eval' or 'translate'", (yyvsp[-2].text));
				YYERROR;
			}
		}
#line 1699 "albumtheme.c"
    break;

  case 24:
#line 258 "albumtheme.y"
                                                             {
			GthTagType tag_type = gth_tag_get_type_from_name ((yyvsp[-2].text));
			if (tag_type == GTH_TAG_INVALID) {
				yyerror ("Unrecognized function: %s", (yyvsp[-2].text));
				YYERROR;
			}
			(yyval.tag) = gth_tag_new (tag_type, (yyvsp[-1].list));
		}
#line 1712 "albumtheme.c"
    break;

  case 25:
#line 268 "albumtheme.y"
                                           {
			(yyval.list) = g_list_prepend ((yyvsp[0].list), (yyvsp[-1].attribute));
		}
#line 1720 "albumtheme.c"
    break;

  case 26:
#line 272 "albumtheme.y"
                              {
			(yyval.list) = NULL;
		}
#line 1728 "albumtheme.c"
    break;

  case 27:
#line 277 "albumtheme.y"
                                                  {
			(yyval.attribute) = gth_attribute_new_expression ((yyvsp[-4].text), (yyvsp[-1].expr));
			g_free ((yyvsp[-4].text));
		}
#line 1737 "albumtheme.c"
    break;

  case 28:
#line 282 "albumtheme.y"
                                                             {
			(yyval.attribute) = gth_attribute_new_string ((yyvsp[-4].text), (yyvsp[-1].text));
			g_free ((yyvsp[-4].text));
			g_free ((yyvsp[-1].text));
		}
#line 1747 "albumtheme.c"
    break;

  case 29:
#line 288 "albumtheme.y"
                                 {
			GthExpr *e = gth_expr_new ();
			gth_expr_push_integer (e, 1);
			(yyval.attribute) = gth_attribute_new_expression ((yyvsp[0].text), e);
			g_free ((yyvsp[0].text));
		}
#line 1758 "albumtheme.c"
    break;

  case 30:
#line 296 "albumtheme.y"
                                     {
			(yyval.list) = g_list_prepend ((yyvsp[0].list), (yyvsp[-2].expr));
		}
#line 1766 "albumtheme.c"
    break;

  case 31:
#line 300 "albumtheme.y"
                       {
			(yyval.list) = g_list_prepend (NULL, (yyvsp[0].expr));
		}
#line 1774 "albumtheme.c"
    break;

  case 32:
#line 304 "albumtheme.y"
                              {
			(yyval.list) = NULL;
		}
#line 1782 "albumtheme.c"
    break;

  case 33:
#line 309 "albumtheme.y"
                               {
			(yyval.expr) = (yyvsp[-1].expr);
		}
#line 1790 "albumtheme.c"
    break;

  case 34:
#line 313 "albumtheme.y"
                                    {
			GthExpr *e = gth_expr_new ();

			gth_expr_push_expr (e, (yyvsp[-2].expr));
			gth_expr_push_expr (e, (yyvsp[0].expr));
			gth_expr_push_op (e, (yyvsp[-1].ivalue));

			gth_expr_unref ((yyvsp[-2].expr));
			gth_expr_unref ((yyvsp[0].expr));

			(yyval.expr) = e;
		}
#line 1807 "albumtheme.c"
    break;

  case 35:
#line 326 "albumtheme.y"
                                {
			GthExpr *e = gth_expr_new ();

			gth_expr_push_expr (e, (yyvsp[-2].expr));
			gth_expr_push_expr (e, (yyvsp[0].expr));
			gth_expr_push_op (e, GTH_OP_ADD);

			gth_expr_unref ((yyvsp[-2].expr));
			gth_expr_unref ((yyvsp[0].expr));

			(yyval.expr) = e;
		}
#line 1824 "albumtheme.c"
    break;

  case 36:
#line 339 "albumtheme.y"
                                {
			GthExpr *e = gth_expr_new ();

			gth_expr_push_expr (e, (yyvsp[-2].expr));
			gth_expr_push_expr (e, (yyvsp[0].expr));
			gth_expr_push_op (e, GTH_OP_SUB);

			gth_expr_unref ((yyvsp[-2].expr));
			gth_expr_unref ((yyvsp[0].expr));

			(yyval.expr) = e;
		}
#line 1841 "albumtheme.c"
    break;

  case 37:
#line 352 "albumtheme.y"
                                {
			GthExpr *e = gth_expr_new ();

			gth_expr_push_expr (e, (yyvsp[-2].expr));
			gth_expr_push_expr (e, (yyvsp[0].expr));
			gth_expr_push_op (e, GTH_OP_MUL);

			gth_expr_unref ((yyvsp[-2].expr));
			gth_expr_unref ((yyvsp[0].expr));

			(yyval.expr) = e;
		}
#line 1858 "albumtheme.c"
    break;

  case 38:
#line 365 "albumtheme.y"
                                {
			GthExpr *e = gth_expr_new ();

			gth_expr_push_expr (e, (yyvsp[-2].expr));
			gth_expr_push_expr (e, (yyvsp[0].expr));
			gth_expr_push_op (e, GTH_OP_DIV);

			gth_expr_unref ((yyvsp[-2].expr));
			gth_expr_unref ((yyvsp[0].expr));

			(yyval.expr) = e;
		}
#line 1875 "albumtheme.c"
    break;

  case 39:
#line 378 "albumtheme.y"
                                    {
			GthExpr *e = gth_expr_new ();

			gth_expr_push_expr (e, (yyvsp[-2].expr));
			gth_expr_push_expr (e, (yyvsp[0].expr));
			gth_expr_push_op (e, (yyvsp[-1].ivalue));

			gth_expr_unref ((yyvsp[-2].expr));
			gth_expr_unref ((yyvsp[0].expr));

			(yyval.expr) = e;
		}
#line 1892 "albumtheme.c"
    break;

  case 40:
#line 391 "albumtheme.y"
                                          {
			(yyval.expr) = (yyvsp[0].expr);
		}
#line 1900 "albumtheme.c"
    break;

  case 41:
#line 395 "albumtheme.y"
                                          {
			gth_expr_push_op ((yyvsp[0].expr), GTH_OP_NEG);
			(yyval.expr) = (yyvsp[0].expr);
		}
#line 1909 "albumtheme.c"
    break;

  case 42:
#line 400 "albumtheme.y"
                                          {
			gth_expr_push_op ((yyvsp[0].expr), GTH_OP_NOT);
			(yyval.expr) = (yyvsp[0].expr);
		}
#line 1918 "albumtheme.c"
    break;

  case 43:
#line 405 "albumtheme.y"
                                                            { /* function call */
			GthExpr *e = gth_expr_new ();
			gth_expr_push_var (e, (yyvsp[-3].text));
			if ((yyvsp[-1].list) != NULL) {
				GList *scan;

				for (scan = (yyvsp[-1].list); scan; scan = scan->next) {
					GthExpr *arg = scan->data;
					gth_expr_push_expr (e, arg);
					gth_expr_unref (arg);
				}
				g_list_free ((yyvsp[-1].list));
			}
			g_free ((yyvsp[-3].text));
			(yyval.expr) = e;
		}
#line 1939 "albumtheme.c"
    break;

  case 44:
#line 422 "albumtheme.y"
                           {
			GthExpr *e = gth_expr_new ();
			gth_expr_push_var (e, (yyvsp[0].text));
			g_free ((yyvsp[0].text));
			(yyval.expr) = e;
		}
#line 1950 "albumtheme.c"
    break;

  case 45:
#line 429 "albumtheme.y"
                                          {
			GthExpr *e = gth_expr_new ();
			gth_expr_push_string (e, (yyvsp[-1].text));
			g_free ((yyvsp[-1].text));
			(yyval.expr) = e;
		}
#line 1961 "albumtheme.c"
    break;

  case 46:
#line 436 "albumtheme.y"
                         {
			GthExpr *e = gth_expr_new ();
			gth_expr_push_integer (e, (yyvsp[0].ivalue));
			(yyval.expr) = e;
		}
#line 1971 "albumtheme.c"
    break;


#line 1975 "albumtheme.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[+*yyssp], yyvsp);
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
  return yyresult;
}
#line 443 "albumtheme.y"



int
gth_albumtheme_yywrap (void)
{
	return 1;
}


void
gth_albumtheme_yyerror (const char *fmt, ...)
{
	va_list ap;

	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	fprintf (stderr, "\n");
	va_end (ap);
}


#include "albumtheme-lex.c"
