#ifndef SITH_SITHCOGPARSE_H
#define SITH_SITHCOGPARSE_H
#include <j3dcore/j3d.h>

#include <rdroid/types.h>

#include <sith/Cog/sithCogExec.h>
#include <sith/Main/sithMain.h>
#include <sith/types.h>

J3D_EXTERN_C_START

void J3DAPI sithCogParse_FreeParseTree(void);
int J3DAPI sithCogParse_Load(const char* pFilename, SithCogScript* pScript, int bParseDescription);

int J3DAPI sithCogParse_ParseSectionCode(SithCogScript* pScript);

SithCogSymbolTable* J3DAPI sithCogParse_DuplicateSymbolTable(const SithCogSymbolTable* pSource);
SithCogSymbolTable* J3DAPI sithCogParse_AllocSymbolTable(size_t numElements);
void J3DAPI sithCogParse_ReallocSymbolTable(SithCogSymbolTable* pTable);
void J3DAPI sithCogParse_FreeSymbolTable(SithCogSymbolTable* pTable);

SithCogSymbol* J3DAPI sithCogParse_AddSymbol(SithCogSymbolTable* pTable, const char* pName);
void J3DAPI sithCogParse_SetSymbolValue(SithCogSymbol* pSymbol, const SithCogSymbolValue* pValue);
SithCogSymbol* J3DAPI sithCogParse_GetSymbol(const SithCogSymbolTable* pLocal, const char* pName);
SithCogSymbol* J3DAPI sithCogParse_GetSymbolByID(const SithCogSymbolTable* pTable, size_t symbolId);
uint32_t J3DAPI sithCogParse_GetSymbolLabel(size_t symbolId);

SithCogSyntaxNode* J3DAPI sithCogParse_MakeLeafNode(SithCogExecOpcode opcode, int symbolId);
SithCogSyntaxNode* J3DAPI sithCogParse_MakeVectorLeafNode(SithCogExecOpcode opcode, const rdVector3* pVect);
SithCogSyntaxNode* J3DAPI sithCogParse_MakeNode(SithCogSyntaxNode* pLeft, SithCogSyntaxNode* pRight, SithCogExecOpcode opcode, int value);

void J3DAPI sithCogParse_LexerSetSymbol(const char* pName);
void J3DAPI sithCogParse_LexerSetString(const char* pString);
void J3DAPI sithCogParse_LexerSetVector(const char* pString);

uint32_t J3DAPI sithCogParse_GetNextLabel(void);
SithCogSyntaxNode* J3DAPI sithCogParse_GetNextNode(void);

void J3DAPI sithCogParse_ResetTreeNodes(SithCogSyntaxNode* pRoot);
void J3DAPI sithCogParse_GenerateLabelTable(const SithCogSyntaxNode* pRoot);
void J3DAPI sithCogParse_GenerateCode(const SithCogSyntaxNode* pNode);

int J3DAPI sithCogParse_ParseSymbols(SithCogScript* pScript, int bParseDescription);
int J3DAPI sithCogParse_ParseSymbolRef(SithCogScript* pScript, SithCogSymbolRefType symbolType, int bCopySymbolDescription);
int J3DAPI sithCogParse_ParseFlex(SithCogScript* pScript, int bParseDescription);
int J3DAPI sithCogParse_ParseInt(SithCogScript* pScript, int bCopyDescription);
int J3DAPI sithCogParse_ParseVector(SithCogScript* pScript, int bParseDescription);
int J3DAPI sithCogParse_ParseMessage(SithCogScript* pScript);
void J3DAPI sithCogParse_ResolveMessageHandlers(SithCogScript* pScript);

J3D_EXTERN_C_END
#endif // SITH_SITHCOGPARSE_H
