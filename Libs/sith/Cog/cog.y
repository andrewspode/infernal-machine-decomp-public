%{
#include <sith/Cog/sithCogExec.h>

#include <stdio.h>
%}

%token IDENTIFIER CONSTANT_INT CONSTANT_FLOAT STRING_LITERAL VECTOR_LITERAL
%token LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP
%token TYPE_NAME
%nonassoc IFX
%token IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN CALL

%nonassoc ELSE

%union {
    void *pointerValue;
    float floatValue;
    int intValue;
    rdVector3 vecValue;
    SithCogSyntaxNode *pNode;
}

%{
extern int yylex(void);
extern size_t yylinenum;
SithCogSyntaxNode* pBranchNode;
SithCogSyntaxNode* pConditionNode;

uint32_t sithCogParse_GetNextLabel(void);
uint32_t J3DAPI sithCogParse_GetSymbolLabel(size_t symbolId);
SithCogSyntaxNode* J3DAPI sithCogParse_MakeLeafNode(SithCogExecOpcode opcode, int symbolId);
SithCogSyntaxNode* J3DAPI sithCogParse_MakeVectorLeafNode(SithCogExecOpcode opcode, const rdVector3* pVect);
SithCogSyntaxNode* J3DAPI sithCogParse_MakeNode(SithCogSyntaxNode* pLeft, SithCogSyntaxNode* pRight, SithCogExecOpcode opcode, int value);
%}

%start statement_list
%type <pNode> primary_expression postfix_expression statement statement_list
%type <pNode> argument_expression_list unary_expression
%type <pNode> multiplicative_expression additive_expression relational_expression
%type <pNode> equality_expression and_expression exclusive_or_expression
%type <pNode> inclusive_or_expression logical_or_expression logical_and_expression
%type <pNode> assignment_expression expression
%type <pNode> labeled_statement compound_statement expression_statement
%type <pNode> selection_statement iteration_statement jump_statement
%type <pNode> unary_operator
%%

primary_expression
    : IDENTIFIER         { $$ = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_PUSHSYMBOL, $<intValue>1); }
    | CONSTANT_INT       { $$ = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_PUSHINT, $<intValue>1); }
    | CONSTANT_FLOAT     { $$ = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_PUSHFLOAT, $<intValue>1); }
    | STRING_LITERAL     { $$ = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_PUSHSYMBOL, $<intValue>1); }
    | VECTOR_LITERAL     { $$ = sithCogParse_MakeVectorLeafNode( SITHCOGEXEC_OPCODE_PUSHVECTOR, &$<vecValue>1); }
    | '(' expression ')' { $$ = $2; }
    ;

postfix_expression
    : primary_expression
    | postfix_expression '[' expression ']'                 { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_ARRAYIDX, 0); }
    | postfix_expression '(' ')'                            { $$ = sithCogParse_MakeNode($1, NULL,  SITHCOGEXEC_OPCODE_CALLFUNC, 0); }
    | postfix_expression '(' argument_expression_list ')'   { $$ = sithCogParse_MakeNode($3, $1,  SITHCOGEXEC_OPCODE_CALLFUNC, 0); }
    ;

argument_expression_list
    : assignment_expression
    | argument_expression_list ',' assignment_expression    { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_NOP, 0); }
    ;

unary_operator
    : '-'                                                   { $$ = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_NEG, 0); }
    | '!'                                                   { $$ = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_CMPFALSE, 0); }
    ;

unary_expression
    : postfix_expression
    | unary_operator unary_expression                       { $$ = sithCogParse_MakeNode($2, $1,  SITHCOGEXEC_OPCODE_NOP, 0); }
    ;

multiplicative_expression
    : unary_expression
    | multiplicative_expression '*' unary_expression        { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_MUL, 0); }
    | multiplicative_expression '/' unary_expression        { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_DIV, 0); }
    | multiplicative_expression '%' unary_expression        { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_MOD, 0); }
    ;

additive_expression
    : multiplicative_expression
    | additive_expression '+' multiplicative_expression     { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_ADD, 0); }
    | additive_expression '-' multiplicative_expression     { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_SUB, 0); }
    ;

relational_expression
    : additive_expression
    | relational_expression '<' additive_expression         { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_CMPLS, 0); }
    | relational_expression '>' additive_expression         { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_CMPGT, 0); }
    | relational_expression LE_OP additive_expression       { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_CMPLE, 0); }
    | relational_expression GE_OP additive_expression       { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_CMPGE, 0); }
    ;

equality_expression
    : relational_expression
    | equality_expression EQ_OP relational_expression       { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_CMPEQ, 0); }
    | equality_expression NE_OP relational_expression       { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_CMPNE, 0); }
    ;

and_expression
    : equality_expression
    | and_expression '&' equality_expression                { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_AND, 0); }
    ;

exclusive_or_expression
    : and_expression
    | exclusive_or_expression '^' and_expression            { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_XOR, 0); }
    ;

inclusive_or_expression
    : exclusive_or_expression
    | inclusive_or_expression '|' exclusive_or_expression   { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_OR, 0); }
    ;

logical_and_expression
    : inclusive_or_expression
    | logical_and_expression AND_OP inclusive_or_expression { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_CMPAND, 0); }
    ;

logical_or_expression
    : logical_and_expression
    | logical_or_expression OR_OP logical_and_expression    { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_CMPOR, 0); }
    ;

assignment_expression
    : logical_or_expression
    | unary_expression '=' assignment_expression            { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_ASSIGN, 0); }
    ;

expression
    : assignment_expression
    | expression ',' assignment_expression                  { $$ = sithCogParse_MakeNode($1, $3,  SITHCOGEXEC_OPCODE_NOP, 0); }
    ;

statement
    : labeled_statement
    | compound_statement
    | expression_statement
    | selection_statement
    | iteration_statement
    | jump_statement
    ;

labeled_statement
    : IDENTIFIER ':' statement                              { 
                                                                $$ = sithCogParse_MakeNode($3, NULL,  SITHCOGEXEC_OPCODE_NOP, 0); 
                                                                $$->parentLabel = sithCogParse_GetSymbolLabel($<intValue>1);
                                                            }
    ;

compound_statement
    : '{' '}'                                               { $$ = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_NOP, 0); }
    | '{' statement_list '}'                                { $$ = $2; }
    ;

statement_list
    : statement
    | statement_list statement                              { $$ = sithCogParse_MakeNode($1, $2,  SITHCOGEXEC_OPCODE_NOP, 0);  }
    ;

expression_statement
    : ';'                                                   { $$ = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_NOP, 0); }
    | expression ';'                                        { /* expression ; */ }
    ;

selection_statement
    : IF '(' expression ')' statement %prec IFX             {
                                                                pBranchNode = sithCogParse_MakeNode($5, NULL,  SITHCOGEXEC_OPCODE_NOP, 0);
                                                                pBranchNode->childLabel = sithCogParse_GetNextLabel();

                                                                pConditionNode = sithCogParse_MakeNode($3, NULL,  SITHCOGEXEC_OPCODE_JZ, pBranchNode->childLabel);

                                                                $$ = sithCogParse_MakeNode(pConditionNode, pBranchNode,  SITHCOGEXEC_OPCODE_NOP, 0);
                                                            }
    | IF '(' expression ')' statement ELSE statement        {
                                                                pBranchNode = sithCogParse_MakeNode($7, NULL,  SITHCOGEXEC_OPCODE_NOP, 0);
                                                                pBranchNode->parentLabel = sithCogParse_GetNextLabel();
                                                                pBranchNode->childLabel = sithCogParse_GetNextLabel();

                                                                pConditionNode = sithCogParse_MakeNode($3, NULL,  SITHCOGEXEC_OPCODE_JZ, pBranchNode->parentLabel);
                                                                pConditionNode = sithCogParse_MakeNode(pConditionNode, $5,  SITHCOGEXEC_OPCODE_JMP, pBranchNode->childLabel);

                                                                $$ = sithCogParse_MakeNode(pConditionNode, pBranchNode,  SITHCOGEXEC_OPCODE_NOP, 0);
                                                            }
    ;

iteration_statement
    : WHILE '(' expression ')' statement                    {
                                                                pConditionNode = sithCogParse_MakeNode($3, NULL,  SITHCOGEXEC_OPCODE_JZ, 0);

                                                                $$ = sithCogParse_MakeNode(pConditionNode, $5,  SITHCOGEXEC_OPCODE_JMP, 0);
                                                                $$->childLabel = sithCogParse_GetNextLabel();
                                                                $$->parentLabel = sithCogParse_GetNextLabel();

                                                                pConditionNode->value = $$->childLabel;
                                                                $$->value = $$->parentLabel;
                                                            }
    | DO statement WHILE '(' expression ')' ';'             {
                                                                $$ = sithCogParse_MakeNode($2, $5,  SITHCOGEXEC_OPCODE_JNZ, 0);
                                                                $$->parentLabel = sithCogParse_GetNextLabel();
                                                                $$->value = $$->parentLabel;
                                                            }
    | FOR '(' expression_statement expression_statement expression ')' statement {
                                                                                     pBranchNode = sithCogParse_MakeNode($7, NULL,  SITHCOGEXEC_OPCODE_NOP, 0);
                                                                                     pBranchNode->childLabel = sithCogParse_GetNextLabel();
                                                                                     
                                                                                     pConditionNode = sithCogParse_MakeNode($4, NULL,  SITHCOGEXEC_OPCODE_JZ, pBranchNode->childLabel);
                                                                                     pConditionNode->parentLabel = sithCogParse_GetNextLabel();

                                                                                     $$ = sithCogParse_MakeNode(pBranchNode, $5,  SITHCOGEXEC_OPCODE_JMP, pConditionNode->parentLabel);

                                                                                     $$->childLabel = sithCogParse_GetNextLabel();
                                                                                     pConditionNode->value = $$->childLabel;

                                                                                     $$ = sithCogParse_MakeNode(sithCogParse_MakeNode($3, pConditionNode,  SITHCOGEXEC_OPCODE_NOP, 0), $$,  SITHCOGEXEC_OPCODE_NOP, 0);
                                                                                 }
    ;

jump_statement
    : GOTO IDENTIFIER ';'       { $$ = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_JMP, sithCogParse_GetSymbolLabel($<intValue>2)); }
    | CALL IDENTIFIER ';'       { $$ = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_CALL, sithCogParse_GetSymbolLabel($<intValue>2)); }
    | RETURN ';'                { $$ = sithCogParse_MakeLeafNode( SITHCOGEXEC_OPCODE_RET, 0); }
    ;

%%

extern char yytext[];
void yyerror(char* s)
{
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "PARSER %s: line %d.\n", s, yylinenum, 0, 0);
}
