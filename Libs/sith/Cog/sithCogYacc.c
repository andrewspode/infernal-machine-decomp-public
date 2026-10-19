#ifndef lint
char yysccsid[] = "@(#)yaccpar	1.4 (Berkeley) 02/25/90";
#endif
#include <sith//Cog//sithCogExec.h>

#include <stdio.h>
typedef union {
    void *pointerValue;
    float floatValue;
    int intValue;
    rdVector3 vecValue;
    SithCogSyntaxNode *pNode;
} YYSTYPE;
extern int yylex(void);
extern size_t yylinenum;
SithCogSyntaxNode* pBranchNode;
SithCogSyntaxNode* pConditionNode;

uint32_t sithCogParse_GetNextLabel(void);
uint32_t J3DAPI sithCogParse_GetSymbolLabel(size_t symbolId);
SithCogSyntaxNode* J3DAPI sithCogParse_MakeLeafNode(SithCogExecOpcode opcode, int symbolId);
SithCogSyntaxNode* J3DAPI sithCogParse_MakeVectorLeafNode(SithCogExecOpcode opcode, const rdVector3* pVect);
SithCogSyntaxNode* J3DAPI sithCogParse_MakeNode(SithCogSyntaxNode* pLeft, SithCogSyntaxNode* pRight, SithCogExecOpcode opcode, int value);
#define IDENTIFIER 257
#define CONSTANT_INT 258
#define CONSTANT_FLOAT 259
#define STRING_LITERAL 260
#define VECTOR_LITERAL 261
#define LE_OP 262
#define GE_OP 263
#define EQ_OP 264
#define NE_OP 265
#define AND_OP 266
#define OR_OP 267
#define TYPE_NAME 268
#define IFX 269
#define IF 270
#define ELSE 271
#define SWITCH 272
#define WHILE 273
#define DO 274
#define FOR 275
#define GOTO 276
#define CONTINUE 277
#define BREAK 278
#define RETURN 279
#define CALL 280
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    1,    1,    1,    1,    1,    1,    2,    2,    2,    2,
    4,    4,   23,   23,    5,    5,    6,    6,    6,    6,
    7,    7,    7,    8,    8,    8,    8,    8,    9,    9,
    9,   10,   10,   11,   11,   12,   12,   14,   14,   13,
   13,   15,   15,   16,   16,    3,    3,    3,    3,    3,
    3,   17,   18,   18,    0,    0,   19,   19,   20,   20,
   21,   21,   21,   22,   22,   22,
};
short yylen[] = {                                         2,
    1,    1,    1,    1,    1,    3,    1,    4,    3,    4,
    1,    3,    1,    1,    1,    2,    1,    3,    3,    3,
    1,    3,    3,    1,    3,    3,    3,    3,    1,    3,
    3,    1,    3,    1,    3,    1,    3,    1,    3,    1,
    3,    1,    3,    1,    3,    1,    1,    1,    1,    1,
    1,    3,    2,    3,    1,    2,    1,    2,    5,    7,
    5,    7,    7,    3,    3,    2,
};
short yydefred[] = {                                      0,
    0,    2,    3,    4,    5,    0,    0,    0,    0,    0,
    0,    0,    0,   13,   14,    0,   57,    0,    7,    0,
   55,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   44,    0,   46,   47,   48,   49,   50,   51,    0,
    0,    0,    0,    0,    0,    0,   66,    0,    1,    0,
   53,    0,   56,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   58,   16,   52,    0,    0,    0,    0,
   64,   65,    6,   54,    9,    0,   11,    0,   43,   18,
   19,   20,   17,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   45,    0,    0,    0,
    0,   10,    0,    8,    0,   61,    0,    0,   12,    0,
    0,    0,   60,   62,   63,
};
short yydgoto[] = {                                      18,
   19,   20,   21,   86,   22,   23,   24,   25,   26,   27,
   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,
   38,   39,   40,
};
short yysindex[] = {                                     34,
  -37,    0,    0,    0,    0,  -11,   -9,   34,   -1, -207,
    5, -182,   98,    0,    0,  -33,    0,   34,    0,  -30,
    0,   25,   -4,  -18,  -58, -209,   58,   13,  -16, -169,
 -152,    0,  -39,    0,    0,    0,    0,    0,    0,   98,
   34,   98,   98, -151,  257,   64,    0,   76,    0,  -26,
    0,    9,    0,   -5,   98,   98,   98,   98,   98,   98,
   98,   98,   98,   98,   98,   98,   98,   98,   98,   98,
   98,   98,   98,    0,    0,    0,    3,    4,   99,  257,
    0,    0,    0,    0,    0,   28,    0,  -36,    0,    0,
    0,    0,    0,   -4,   -4,  -18,  -18,  -18,  -18,  -58,
  -58, -209,   58,   13, -152,  -16,    0,   34,   34,   98,
   98,    0,   98,    0, -131,    0,   43,   92,    0,   34,
   82,   34,    0,    0,    0,
};
short yyrindex[] = {                                      0,
   57,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   68,
    0,  126,  115,  360,  -35,  461,  387,  491,  -22,  -27,
   44,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  301,  309,  367,  377,  403,  414,  428,
  454,  465,  487,  558,  106,   32,    0,    0,    0,    0,
    0,    0,    0,    0,    1,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,
};
short yygindex[] = {                                    128,
    0,    0,  492,    0,  643,    2,   18,   11,   74,   77,
   75,   80,    0,   78,  -43,   10,    0,    0,  -29,    0,
    0,    0,    0,
};
#define YYTABLESIZE 825
short yytable[] = {                                      15,
   59,   64,   29,   65,   73,   29,   13,   73,   29,   54,
   87,   14,   89,   42,   83,   80,   42,   73,   38,   74,
   41,   38,   50,   29,   61,   17,   60,   15,   42,  107,
   43,   42,   59,   59,   13,   85,   38,   57,   45,   14,
   59,   15,   58,  108,  109,   59,   73,   73,   13,   46,
  111,   77,   78,   14,   66,   67,  114,   29,   29,   59,
   55,   94,   95,   47,   88,   42,   15,   17,  112,  119,
   38,  113,   39,   13,   48,   39,  100,  101,   14,   96,
   97,   98,   99,  121,   40,   56,   73,   40,   29,   16,
   39,   51,   17,    1,    1,   68,    1,   71,    1,    1,
    1,    1,   40,    1,   15,   15,   69,   70,   15,   15,
   15,   15,   15,   72,   15,    1,    1,    1,    1,  117,
  118,   79,   81,   59,   39,   59,   15,   15,   15,   15,
   15,   16,  122,   84,   82,   73,   40,   13,  110,  120,
  124,  102,   14,   52,  104,  103,   41,    1,  105,   41,
    1,  106,   21,    0,    0,   21,   16,   21,   21,   21,
   15,   15,   17,   17,   41,    0,   17,   17,   17,   17,
   17,    0,   17,   21,   21,    0,   21,    0,    0,    0,
    1,    0,    0,    0,   17,   17,    0,   17,    0,    0,
    0,   15,    0,    0,    0,    0,    0,    0,   41,    0,
    0,    0,    0,   62,   63,    0,    0,   21,   21,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   17,   17,
    0,    0,    0,    1,    2,    3,    4,    5,   29,   29,
   29,   29,    0,    0,    0,    0,    6,    0,   21,    7,
    8,    9,   10,   38,   38,   11,   12,    0,    0,   17,
    0,   49,    2,    3,    4,    5,    0,   59,   59,   59,
   59,   59,    0,    0,    0,    1,    2,    3,    4,    5,
   59,    0,    0,   59,   59,   59,   59,    0,    6,   59,
   59,    7,    8,    9,   10,    0,    0,   11,   12,   15,
    1,    2,    3,    4,    5,    0,   13,   39,   39,    0,
    0,   14,    0,    6,    0,    0,    7,    8,    9,   10,
   40,    0,   11,   12,    0,   17,    0,    0,    1,    1,
    1,    1,    1,    1,    0,    0,    0,    0,    0,   15,
   15,   15,   15,   15,   15,    0,    0,    0,   23,    0,
    0,   23,    0,   23,   23,   23,   22,    0,    0,   22,
    0,   22,   22,   22,   49,    2,    3,    4,    5,   23,
   23,    0,   23,    0,    0,    0,    0,   22,   22,    0,
   22,    0,   41,    0,    0,    0,   21,   21,   21,   21,
   21,   21,    0,    0,    0,    0,    0,   17,   17,   17,
   17,   17,   17,   23,   23,    0,    0,   24,    0,    0,
   24,   22,   22,   24,   27,    0,    0,   27,    0,    0,
   27,    0,    0,    0,   28,    0,    0,   28,   24,   24,
   28,   24,    0,    0,   23,   27,   27,   34,   27,    0,
   34,    0,   22,    0,    0,   28,   28,    0,   28,    0,
   25,    0,    0,   25,    0,   34,   25,    0,    0,    0,
    0,   26,   24,   24,   26,    0,    0,   26,    0,   27,
   27,   25,   25,    0,   25,   30,    0,    0,   30,   28,
   28,   30,   26,   26,    0,   26,    0,    0,    0,   34,
   34,    0,    0,   24,    0,    0,   30,    0,    0,    0,
   27,   31,    0,    0,   31,   25,   25,   31,   32,   44,
   28,   32,   33,    0,   32,   33,   26,   26,   33,   53,
   34,    0,   31,   49,    2,    3,    4,    5,    0,   32,
   30,   30,    0,   33,    0,    0,   25,   35,    0,    0,
   35,   36,   76,    0,   36,    0,    0,   26,    0,    0,
    0,    0,    0,   53,    0,   35,   31,   31,    0,   36,
    0,   30,    0,   32,   32,    0,    0,   33,   33,    0,
    0,    0,   23,   23,   23,   23,   23,   23,    0,    0,
   22,   22,   22,   22,   22,   22,    0,   31,    0,   35,
   35,    0,    0,   36,   32,    0,    0,    0,   33,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   37,  115,
  116,   37,    0,    0,    0,    0,    0,    0,    0,    0,
   35,  123,    0,  125,   36,    0,   37,    0,    0,    0,
    0,   24,   24,   24,   24,   24,   24,    0,   27,   27,
   27,   27,   27,   27,    0,    0,    0,    0,   28,   28,
   28,   28,   28,   28,    0,    0,    0,    0,    0,    0,
   37,    0,   34,   34,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   25,   25,   25,   25,   25,   25,
    0,    0,    0,    0,    0,   26,   26,   26,   26,   26,
   26,   37,   75,    0,    0,    0,    0,    0,    0,    0,
    0,   30,   30,   30,   30,    0,    0,    0,    0,   90,
   91,   92,   93,   93,   93,   93,   93,   93,   93,   93,
   93,   93,   93,   93,   93,    0,    0,   31,   31,   31,
   31,    0,    0,    0,    0,    0,   32,   32,    0,    0,
   33,   33,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   35,   35,    0,    0,   36,   36,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   37,   37,
};
short yycheck[] = {                                      33,
    0,   60,   38,   62,   44,   41,   40,   44,   44,   40,
   54,   45,   56,   41,   41,   45,   44,   44,   41,   59,
   58,   44,   13,   59,   43,   59,   45,   33,   40,   73,
   40,   59,   37,   33,   40,   41,   59,   42,   40,   45,
   40,   33,   47,   41,   41,   45,   44,   44,   40,  257,
   80,   42,   43,   45,  264,  265,   93,   93,   94,   59,
   91,   60,   61,   59,   55,   93,   33,   59,   41,  113,
   93,   44,   41,   40,  257,   44,   66,   67,   45,   62,
   63,   64,   65,   41,   41,   61,   44,   44,  124,  123,
   59,  125,   59,   37,   38,   38,   40,  267,   42,   43,
   44,   45,   59,   47,   37,   38,   94,  124,   41,   42,
   43,   44,   45,  266,   47,   59,   60,   61,   62,  110,
  111,  273,   59,  123,   93,  125,   59,   60,   61,   62,
   33,  123,   41,  125,   59,   44,   93,   40,   40,  271,
   59,   68,   45,   16,   70,   69,   41,   91,   71,   44,
   94,   72,   38,   -1,   -1,   41,  123,   43,   44,   45,
   93,   94,   37,   38,   59,   -1,   41,   42,   43,   44,
   45,   -1,   47,   59,   60,   -1,   62,   -1,   -1,   -1,
  124,   -1,   -1,   -1,   59,   60,   -1,   62,   -1,   -1,
   -1,  124,   -1,   -1,   -1,   -1,   -1,   -1,   93,   -1,
   -1,   -1,   -1,  262,  263,   -1,   -1,   93,   94,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   93,   94,
   -1,   -1,   -1,  257,  258,  259,  260,  261,  264,  265,
  266,  267,   -1,   -1,   -1,   -1,  270,   -1,  124,  273,
  274,  275,  276,  266,  267,  279,  280,   -1,   -1,  124,
   -1,  257,  258,  259,  260,  261,   -1,  257,  258,  259,
  260,  261,   -1,   -1,   -1,  257,  258,  259,  260,  261,
  270,   -1,   -1,  273,  274,  275,  276,   -1,  270,  279,
  280,  273,  274,  275,  276,   -1,   -1,  279,  280,   33,
  257,  258,  259,  260,  261,   -1,   40,  266,  267,   -1,
   -1,   45,   -1,  270,   -1,   -1,  273,  274,  275,  276,
  267,   -1,  279,  280,   -1,   59,   -1,   -1,  262,  263,
  264,  265,  266,  267,   -1,   -1,   -1,   -1,   -1,  262,
  263,  264,  265,  266,  267,   -1,   -1,   -1,   38,   -1,
   -1,   41,   -1,   43,   44,   45,   38,   -1,   -1,   41,
   -1,   43,   44,   45,  257,  258,  259,  260,  261,   59,
   60,   -1,   62,   -1,   -1,   -1,   -1,   59,   60,   -1,
   62,   -1,  267,   -1,   -1,   -1,  262,  263,  264,  265,
  266,  267,   -1,   -1,   -1,   -1,   -1,  262,  263,  264,
  265,  266,  267,   93,   94,   -1,   -1,   38,   -1,   -1,
   41,   93,   94,   44,   38,   -1,   -1,   41,   -1,   -1,
   44,   -1,   -1,   -1,   38,   -1,   -1,   41,   59,   60,
   44,   62,   -1,   -1,  124,   59,   60,   41,   62,   -1,
   44,   -1,  124,   -1,   -1,   59,   60,   -1,   62,   -1,
   38,   -1,   -1,   41,   -1,   59,   44,   -1,   -1,   -1,
   -1,   38,   93,   94,   41,   -1,   -1,   44,   -1,   93,
   94,   59,   60,   -1,   62,   38,   -1,   -1,   41,   93,
   94,   44,   59,   60,   -1,   62,   -1,   -1,   -1,   93,
   94,   -1,   -1,  124,   -1,   -1,   59,   -1,   -1,   -1,
  124,   38,   -1,   -1,   41,   93,   94,   44,   38,    8,
  124,   41,   38,   -1,   44,   41,   93,   94,   44,   18,
  124,   -1,   59,  257,  258,  259,  260,  261,   -1,   59,
   93,   94,   -1,   59,   -1,   -1,  124,   41,   -1,   -1,
   44,   41,   41,   -1,   44,   -1,   -1,  124,   -1,   -1,
   -1,   -1,   -1,   52,   -1,   59,   93,   94,   -1,   59,
   -1,  124,   -1,   93,   94,   -1,   -1,   93,   94,   -1,
   -1,   -1,  262,  263,  264,  265,  266,  267,   -1,   -1,
  262,  263,  264,  265,  266,  267,   -1,  124,   -1,   93,
   94,   -1,   -1,   93,  124,   -1,   -1,   -1,  124,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   41,  108,
  109,   44,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  124,  120,   -1,  122,  124,   -1,   59,   -1,   -1,   -1,
   -1,  262,  263,  264,  265,  266,  267,   -1,  262,  263,
  264,  265,  266,  267,   -1,   -1,   -1,   -1,  262,  263,
  264,  265,  266,  267,   -1,   -1,   -1,   -1,   -1,   -1,
   93,   -1,  266,  267,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  262,  263,  264,  265,  266,  267,
   -1,   -1,   -1,   -1,   -1,  262,  263,  264,  265,  266,
  267,  124,   40,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  264,  265,  266,  267,   -1,   -1,   -1,   -1,   57,
   58,   59,   60,   61,   62,   63,   64,   65,   66,   67,
   68,   69,   70,   71,   72,   -1,   -1,  264,  265,  266,
  267,   -1,   -1,   -1,   -1,   -1,  266,  267,   -1,   -1,
  266,  267,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  266,  267,   -1,   -1,  266,  267,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  266,  267,
};
#define YYFINAL 18
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 280
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"'!'",0,0,0,"'%'","'&'",0,"'('","')'","'*'","'+'","','","'-'",0,"'/'",0,0,0,0,0,
0,0,0,0,0,"':'","';'","'<'","'='","'>'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,"'['",0,"']'","'^'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"'{'","'|'","'}'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"IDENTIFIER","CONSTANT_INT",
"CONSTANT_FLOAT","STRING_LITERAL","VECTOR_LITERAL","LE_OP","GE_OP","EQ_OP",
"NE_OP","AND_OP","OR_OP","TYPE_NAME","IFX","IF","ELSE","SWITCH","WHILE","DO",
"FOR","GOTO","CONTINUE","BREAK","RETURN","CALL",
};
char *yyrule[] = {
"$accept : statement_list",
"primary_expression : IDENTIFIER",
"primary_expression : CONSTANT_INT",
"primary_expression : CONSTANT_FLOAT",
"primary_expression : STRING_LITERAL",
"primary_expression : VECTOR_LITERAL",
"primary_expression : '(' expression ')'",
"postfix_expression : primary_expression",
"postfix_expression : postfix_expression '[' expression ']'",
"postfix_expression : postfix_expression '(' ')'",
"postfix_expression : postfix_expression '(' argument_expression_list ')'",
"argument_expression_list : assignment_expression",
"argument_expression_list : argument_expression_list ',' assignment_expression",
"unary_operator : '-'",
"unary_operator : '!'",
"unary_expression : postfix_expression",
"unary_expression : unary_operator unary_expression",
"multiplicative_expression : unary_expression",
"multiplicative_expression : multiplicative_expression '*' unary_expression",
"multiplicative_expression : multiplicative_expression '/' unary_expression",
"multiplicative_expression : multiplicative_expression '%' unary_expression",
"additive_expression : multiplicative_expression",
"additive_expression : additive_expression '+' multiplicative_expression",
"additive_expression : additive_expression '-' multiplicative_expression",
"relational_expression : additive_expression",
"relational_expression : relational_expression '<' additive_expression",
"relational_expression : relational_expression '>' additive_expression",
"relational_expression : relational_expression LE_OP additive_expression",
"relational_expression : relational_expression GE_OP additive_expression",
"equality_expression : relational_expression",
"equality_expression : equality_expression EQ_OP relational_expression",
"equality_expression : equality_expression NE_OP relational_expression",
"and_expression : equality_expression",
"and_expression : and_expression '&' equality_expression",
"exclusive_or_expression : and_expression",
"exclusive_or_expression : exclusive_or_expression '^' and_expression",
"inclusive_or_expression : exclusive_or_expression",
"inclusive_or_expression : inclusive_or_expression '|' exclusive_or_expression",
"logical_and_expression : inclusive_or_expression",
"logical_and_expression : logical_and_expression AND_OP inclusive_or_expression",
"logical_or_expression : logical_and_expression",
"logical_or_expression : logical_or_expression OR_OP logical_and_expression",
"assignment_expression : logical_or_expression",
"assignment_expression : unary_expression '=' assignment_expression",
"expression : assignment_expression",
"expression : expression ',' assignment_expression",
"statement : labeled_statement",
"statement : compound_statement",
"statement : expression_statement",
"statement : selection_statement",
"statement : iteration_statement",
"statement : jump_statement",
"labeled_statement : IDENTIFIER ':' statement",
"compound_statement : '{' '}'",
"compound_statement : '{' statement_list '}'",
"statement_list : statement",
"statement_list : statement_list statement",
"expression_statement : ';'",
"expression_statement : expression ';'",
"selection_statement : IF '(' expression ')' statement",
"selection_statement : IF '(' expression ')' statement ELSE statement",
"iteration_statement : WHILE '(' expression ')' statement",
"iteration_statement : DO statement WHILE '(' expression ')' ';'",
"iteration_statement : FOR '(' expression_statement expression_statement expression ')' statement",
"jump_statement : GOTO IDENTIFIER ';'",
"jump_statement : CALL IDENTIFIER ';'",
"jump_statement : RETURN ';'",
};
#endif
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#ifndef YYSTACKSIZE
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 300
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
#define yystacksize YYSTACKSIZE
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];

extern char yytext[];
void yyerror(char* s)
{
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "PARSER %s: line %d.\n", s, yylinenum, 0, 0);
}
#define YYABORT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("yydebug: state %d, reading %d (%s)\n", yystate,
                    yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("yydebug: state %d, shifting to state %d\n",
                    yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("yydebug: state %d, error recovery shifting\
 to state %d\n", *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("yydebug: error recovery discarding state %d\n",
                            *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("yydebug: state %d, error recovery discards token %d (%s)\n",
                    yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("yydebug: state %d, reducing by rule %d (%s)\n",
                yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
{ yyval.pNode  = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_PUSHSYMBOL, yyvsp[0].intValue ); }
break;
case 2:
{ yyval.pNode  = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_PUSHINT, yyvsp[0].intValue ); }
break;
case 3:
{ yyval.pNode  = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_PUSHFLOAT, yyvsp[0].intValue ); }
break;
case 4:
{ yyval.pNode  = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_PUSHSYMBOL, yyvsp[0].intValue ); }
break;
case 5:
{ yyval.pNode  = sithCogParse_MakeVectorLeafNode( SITHCOGEXEC_OPCODE_PUSHVECTOR, &yyvsp[0].vecValue ); }
break;
case 6:
{ yyval.pNode  = yyvsp[-1].pNode ; }
break;
case 8:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-3].pNode , yyvsp[-1].pNode ,  SITHCOGEXEC_OPCODE_ARRAYIDX, 0); }
break;
case 9:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , NULL,  SITHCOGEXEC_OPCODE_CALLFUNC, 0); }
break;
case 10:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-1].pNode , yyvsp[-3].pNode ,  SITHCOGEXEC_OPCODE_CALLFUNC, 0); }
break;
case 12:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_NOP, 0); }
break;
case 13:
{ yyval.pNode  = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_NEG, 0); }
break;
case 14:
{ yyval.pNode  = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_CMPFALSE, 0); }
break;
case 16:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[0].pNode , yyvsp[-1].pNode ,  SITHCOGEXEC_OPCODE_NOP, 0); }
break;
case 18:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_MUL, 0); }
break;
case 19:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_DIV, 0); }
break;
case 20:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_MOD, 0); }
break;
case 22:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_ADD, 0); }
break;
case 23:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_SUB, 0); }
break;
case 25:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_CMPLS, 0); }
break;
case 26:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_CMPGT, 0); }
break;
case 27:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_CMPLE, 0); }
break;
case 28:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_CMPGE, 0); }
break;
case 30:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_CMPEQ, 0); }
break;
case 31:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_CMPNE, 0); }
break;
case 33:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_AND, 0); }
break;
case 35:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_XOR, 0); }
break;
case 37:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_OR, 0); }
break;
case 39:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_CMPAND, 0); }
break;
case 41:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_CMPOR, 0); }
break;
case 43:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_ASSIGN, 0); }
break;
case 45:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-2].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_NOP, 0); }
break;
case 52:
{ 
                                                                yyval.pNode  = sithCogParse_MakeNode(yyvsp[0].pNode , NULL,  SITHCOGEXEC_OPCODE_NOP, 0); 
                                                                yyval.pNode ->parentLabel = sithCogParse_GetSymbolLabel(yyvsp[-2].intValue );
                                                            }
break;
case 53:
{ yyval.pNode  = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_NOP, 0); }
break;
case 54:
{ yyval.pNode  = yyvsp[-1].pNode ; }
break;
case 56:
{ yyval.pNode  = sithCogParse_MakeNode(yyvsp[-1].pNode , yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_NOP, 0);  }
break;
case 57:
{ yyval.pNode  = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_NOP, 0); }
break;
case 58:
{ /* expression ; */ }
break;
case 59:
{
                                                                pBranchNode = sithCogParse_MakeNode(yyvsp[0].pNode , NULL,  SITHCOGEXEC_OPCODE_NOP, 0);
                                                                pBranchNode->childLabel = sithCogParse_GetNextLabel();

                                                                pConditionNode = sithCogParse_MakeNode(yyvsp[-2].pNode , NULL,  SITHCOGEXEC_OPCODE_JZ, pBranchNode->childLabel);

                                                                yyval.pNode  = sithCogParse_MakeNode(pConditionNode, pBranchNode,  SITHCOGEXEC_OPCODE_NOP, 0);
                                                            }
break;
case 60:
{
                                                                pBranchNode = sithCogParse_MakeNode(yyvsp[0].pNode , NULL,  SITHCOGEXEC_OPCODE_NOP, 0);
                                                                pBranchNode->parentLabel = sithCogParse_GetNextLabel();
                                                                pBranchNode->childLabel = sithCogParse_GetNextLabel();

                                                                pConditionNode = sithCogParse_MakeNode(yyvsp[-4].pNode , NULL,  SITHCOGEXEC_OPCODE_JZ, pBranchNode->parentLabel);
                                                                pConditionNode = sithCogParse_MakeNode(pConditionNode, yyvsp[-2].pNode ,  SITHCOGEXEC_OPCODE_JMP, pBranchNode->childLabel);

                                                                yyval.pNode  = sithCogParse_MakeNode(pConditionNode, pBranchNode,  SITHCOGEXEC_OPCODE_NOP, 0);
                                                            }
break;
case 61:
{
                                                                pConditionNode = sithCogParse_MakeNode(yyvsp[-2].pNode , NULL,  SITHCOGEXEC_OPCODE_JZ, 0);

                                                                yyval.pNode  = sithCogParse_MakeNode(pConditionNode, yyvsp[0].pNode ,  SITHCOGEXEC_OPCODE_JMP, 0);
                                                                yyval.pNode ->childLabel = sithCogParse_GetNextLabel();
                                                                yyval.pNode ->parentLabel = sithCogParse_GetNextLabel();

                                                                pConditionNode->value = yyval.pNode ->childLabel;
                                                                yyval.pNode ->value = yyval.pNode ->parentLabel;
                                                            }
break;
case 62:
{
                                                                yyval.pNode  = sithCogParse_MakeNode(yyvsp[-5].pNode , yyvsp[-2].pNode ,  SITHCOGEXEC_OPCODE_JNZ, 0);
                                                                yyval.pNode ->parentLabel = sithCogParse_GetNextLabel();
                                                                yyval.pNode ->value = yyval.pNode ->parentLabel;
                                                            }
break;
case 63:
{
                                                                                     pBranchNode = sithCogParse_MakeNode(yyvsp[0].pNode , NULL,  SITHCOGEXEC_OPCODE_NOP, 0);
                                                                                     pBranchNode->childLabel = sithCogParse_GetNextLabel();
                                                                                     
                                                                                     pConditionNode = sithCogParse_MakeNode(yyvsp[-3].pNode , NULL,  SITHCOGEXEC_OPCODE_JZ, pBranchNode->childLabel);
                                                                                     pConditionNode->parentLabel = sithCogParse_GetNextLabel();

                                                                                     yyval.pNode  = sithCogParse_MakeNode(pBranchNode, yyvsp[-2].pNode ,  SITHCOGEXEC_OPCODE_JMP, pConditionNode->parentLabel);

                                                                                     yyval.pNode ->childLabel = sithCogParse_GetNextLabel();
                                                                                     pConditionNode->value = yyval.pNode ->childLabel;

                                                                                     yyval.pNode  = sithCogParse_MakeNode(sithCogParse_MakeNode(yyvsp[-4].pNode , pConditionNode,  SITHCOGEXEC_OPCODE_NOP, 0), yyval.pNode ,  SITHCOGEXEC_OPCODE_NOP, 0);
                                                                                 }
break;
case 64:
{ yyval.pNode  = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_JMP, sithCogParse_GetSymbolLabel(yyvsp[-1].intValue )); }
break;
case 65:
{ yyval.pNode  = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_CALL, sithCogParse_GetSymbolLabel(yyvsp[-1].intValue )); }
break;
case 66:
{ yyval.pNode  = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_RET, 0); }
break;
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#ifdef YYDEBUG
        if (yydebug)
            printf("yydebug: after reduction, shifting from state 0 to\
 state %d\n", YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("yydebug: state %d, reading %d (%s)\n",
                        YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#ifdef YYDEBUG
    if (yydebug)
        printf("yydebug: after reduction, shifting from state %d \
to state %d\n", *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
