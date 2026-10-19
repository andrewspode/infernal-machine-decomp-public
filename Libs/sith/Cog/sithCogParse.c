#include "sithCogParse.h"

#include <rdroid/Math/rdVector.h>
#include <rdroid/types.h>

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Cog/sithCogYacc.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/std.h>
#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SITHCOGPARSE_SYNTAXTREESIZE 8096u

static int nextlabel      = 1;
static int bSkipYYRestart = 1;

static SithCogSymbolTable* pCurrentSymbols = 0;
static int32_t aLabelTable[1024]           = { 0 };
static int32_t* pCurrentCode               = 0;
static SithCogSyntaxNode* pCurrentRoot     = 0;
static SithCogSyntaxNode* pSyntaxTree      = 0;
static SithCogScript* pCurrentScript       = 0;
static uint32_t codepos                    = 0;
static size_t numTreeNodes                 = 0;
static size_t syntaxTreeSize               = 0;

void J3DAPI sithCogParse_FreeParseTree(void)
{
    if ( pSyntaxTree )
    {
        stdMemory_Free(pSyntaxTree);
        syntaxTreeSize = 0;
        numTreeNodes   = 0;
    }
}

// Retail jumps from every failure site straight into one of three shared error blocks
// placed after the success return: allocation (line 206), EOF (210, three sites) and
// syntax (214, four sites), which share the cleanup. Written with one label per shared
// block, the text loaders' shared-error-block `goto` style.
//
int J3DAPI sithCogParse_Load(const char* pFilename, SithCogScript* pScript, int bParseDescription)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;

#line 138
    SITH_ASSERTREL(pScript != NULL);
    SITH_ASSERTREL(!pScript->pSymbolTable);

    if ( !stdConffile_Open(pFilename) )
    {
        goto open_error;
    }

    memset(pScript, 0, sizeof(SithCogScript));
    strncpy(pScript->aName, stdFileFromPath(pFilename), STD_ARRAYLEN(pScript->aName) - 1);
    pScript->aName[STD_ARRAYLEN(pScript->aName) - 1] = 0;

    memset(aLabelTable, -1, sizeof(aLabelTable));
    nextlabel = 1;

    if ( !stdConffile_ReadArgs() )
    {
        goto eof_error;
    }

    if ( strcmp(stdConffile_entry.aArgs[0].argName, "flags") != 0 )
    {
    }
    else
    {
        sscanf(stdConffile_entry.aArgs[0].argValue, "%x", &pScript->flags);
        if ( !stdConffile_ReadArgs() )
        {
            goto eof_error;
        }
    }

    if ( strcmp(stdConffile_entry.aArgs[0].argValue, "symbols") != 0 )
    {
        goto syntax_error;
    }

    pScript->pSymbolTable = sithCogParse_AllocSymbolTable(SITHCOGSCRIPT_MAXSYMREFS);
    if ( !pScript->pSymbolTable )
    {
        goto alloc_error;
    }

    if ( !sithCogParse_ParseSymbols(pScript, bParseDescription) )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadArgs() )
    {
        goto eof_error;
    }

    if ( strcmp(stdConffile_entry.aArgs[0].argValue, "code") != 0 )
    {
        goto syntax_error;
    }

    if ( !sithCogParse_ParseSectionCode(pScript) )
    {
        goto syntax_error;
    }

    sithCogParse_ResolveMessageHandlers(pScript);
    stdConffile_Close();
    return 1;

open_error:
#line 202
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Open failed for file %s.\n", pFilename, 0, 0, 0);
    return 0;

alloc_error:
#line 206
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory alloc failed.\n", 0, 0, 0, 0);
    goto error;

eof_error:
#line 210
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Unexpected EOF.\n", 0, 0, 0, 0);
    goto error;

syntax_error:
#line 214
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Syntax error.\n", 0, 0, 0, 0);

error:
    if ( pScript->pSymbolTable )
    {
        sithCogParse_FreeSymbolTable(pScript->pSymbolTable);
        pScript->pSymbolTable = NULL;
    }

    if ( pCurrentRoot )
    {
        sithCogParse_ResetTreeNodes(pCurrentRoot);
        pCurrentRoot = NULL;
    }

    stdConffile_Close();
    return 0;
}

int J3DAPI sithCogParse_ParseSectionCode(SithCogScript* pScript)
{
    tFileHandle currentfh;
    int ret;

#line 258
    SITH_ASSERTREL(pCurrentRoot == NULL);

    if ( (currentfh = stdConffile_GetFileHandle()) == NULL )
    {
    }
    else
    {
        pCurrentSymbols = pScript->pSymbolTable;
        pCurrentScript  = pScript;
        yyin            = currentfh;
        if ( !bSkipYYRestart )
        {
            yyrestart(currentfh);
        }
        else
        {
            bSkipYYRestart = 0;
        }

        yylinenum = 1;
        if ( (ret = yyparse()) != 0 )
        {
#line 288
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "COG parse returned %d.\n", ret, 0, 0, 0);
        }
        else
        {
            codepos = 0;
            sithCogParse_GenerateLabelTable(pCurrentRoot);
#line 309
            if ( (pScript->pCode = (int32_t*)STDMALLOC((sizeof(int32_t) * codepos) + sizeof(int32_t))) == NULL )
            {
#line 312
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Malloc error for COG code.\n", 0, 0, 0, 0);
            }
            else
            {
                pScript->codeSize = codepos + 1;

                codepos      = 0;
                pCurrentCode = pScript->pCode;
                sithCogParse_GenerateCode(pCurrentRoot);

                pScript->pCode[pScript->codeSize - 1] = SITHCOGEXEC_OPCODE_RET;

                sithCogParse_ResetTreeNodes(pCurrentRoot);
                pCurrentRoot = NULL;
                return 1;
            }
        }
    }

    if ( pCurrentRoot )
    {
        sithCogParse_ResetTreeNodes(pCurrentRoot);
        pCurrentRoot = NULL;
    }

    codepos = 0;
    return 0;
}

SithCogSymbolTable* J3DAPI sithCogParse_DuplicateSymbolTable(const SithCogSymbolTable* pSource)
{
    SithCogSymbolTable* pTable;
    size_t numUsed;

#line 366
    SITH_ASSERTREL(pSource);

    numUsed = pSource->numUsedSymbols;
#line 372
    if ( (pTable = (SithCogSymbolTable*)STDMALLOC(sizeof(SithCogSymbolTable))) == NULL )
    {
    }
    else if ( (memset(pTable, 0, sizeof(SithCogSymbolTable)),
#line 380
               pTable->aSymbols = (SithCogSymbol*)STDMALLOC(sizeof(SithCogSymbol) * numUsed)) == NULL )
    {
    }
    else
    {
        memcpy(pTable->aSymbols, pSource->aSymbols, sizeof(SithCogSymbol) * numUsed);
        pTable->tableSize      = numUsed;
        pTable->numUsedSymbols = numUsed;
        pTable->bIsCopy        = 1;
        return pTable;
    }

#line 395
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error while duplicating symbol table.\n", 0, 0, 0, 0);
    return NULL;
}

SithCogSymbolTable* J3DAPI sithCogParse_AllocSymbolTable(size_t numElements)
{
    SithCogSymbolTable* pTable;

#line 421
    SITH_ASSERTREL(numElements > 0);
#line 424
    if ( (pTable = (SithCogSymbolTable*)STDMALLOC(sizeof(SithCogSymbolTable))) == NULL )
    {
    }
    else
    {
        memset(pTable, 0, sizeof(SithCogSymbolTable));
#line 430
        pTable->aSymbols = (SithCogSymbol*)STDMALLOC(sizeof(SithCogSymbol) * numElements);
        pTable->pHashtbl = stdHashtbl_New(2 * numElements);
        if ( !pTable->aSymbols || !pTable->pHashtbl )
        {
        }
        else
        {
            memset(pTable->aSymbols, 0, sizeof(SithCogSymbol) * numElements);
            pTable->tableSize      = numElements;
            pTable->numUsedSymbols = 0;
            pTable->bIsCopy        = 0;
            return pTable;
        }
    }

#line 444
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to create memory for symbol table.\n", 0, 0, 0, 0);
    if ( pTable )
    {
        if ( pTable->aSymbols )
        {
            stdMemory_Free(pTable->aSymbols);
        }

        if ( pTable->pHashtbl )
        {
            stdHashtbl_Free(pTable->pHashtbl);
        }

        stdMemory_Free(pTable);
    }

    return NULL;
}

void J3DAPI sithCogParse_ReallocSymbolTable(SithCogSymbolTable* pTable)
{
    size_t i;

#line 485
    SITH_ASSERTREL(pTable != NULL);
    if ( pTable->pHashtbl )
    {
        stdHashtbl_Free(pTable->pHashtbl);
        pTable->pHashtbl = NULL;
    }

    if ( pTable->tableSize > pTable->numUsedSymbols )
    {
#line 497
        pTable->aSymbols = (SithCogSymbol*)STDREALLOC(pTable->aSymbols, sizeof(SithCogSymbol) * pTable->numUsedSymbols);
        pTable->tableSize = pTable->numUsedSymbols;
    }

    for ( i = 0; i < pTable->tableSize; ++i )
    {
        if ( pTable->aSymbols[i].pName )
        {
            stdMemory_Free(pTable->aSymbols[i].pName);
            pTable->aSymbols[i].pName = NULL;
        }
    }
}

void J3DAPI sithCogParse_FreeSymbolTable(SithCogSymbolTable* pTable)
{
    size_t i;

#line 530
    SITH_ASSERTREL(pTable != NULL);
    if ( pTable->pHashtbl )
    {
        stdHashtbl_Free(pTable->pHashtbl);
        pTable->pHashtbl = NULL;
    }

    if ( pTable->aSymbols )
    {
#line 541
        SITH_ASSERTREL(pTable->tableSize > 0);
        if ( !pTable->bIsCopy )
        {
            for ( i = 0; i < pTable->tableSize; ++i )
            {
                if ( pTable->aSymbols[i].pName )
                {
                    stdMemory_Free(pTable->aSymbols[i].pName);
                }

                if ( pTable->aSymbols[i].value.type == SITHCOG_VALUE_STRING )
                {
#line 555
                    SITH_ASSERTREL(pTable->aSymbols[i].value.val.pString);
                    stdMemory_Free(pTable->aSymbols[i].value.val.pString);
                    pTable->aSymbols[i].value.val.pString = NULL;
                }
            }
        }

        stdMemory_Free(pTable->aSymbols);
        pTable->aSymbols = NULL;
    }

    stdMemory_Free(pTable);
}

SithCogSymbol* J3DAPI sithCogParse_AddSymbol(SithCogSymbolTable* pTable, const char* pName)
{
    SithCogSymbol* pSymbol;

#line 591
    SITH_ASSERTREL(pTable != NULL);
    SITH_ASSERTREL(pTable->aSymbols != NULL);

    if ( pTable->tableSize <= pTable->numUsedSymbols )
    {
#line 596
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No space for COG symbol %s.\n", pName, 0, 0, 0);
        return NULL;
    }

    pSymbol = &pTable->aSymbols[pTable->numUsedSymbols++];
    if ( pName )
    {
#line 606
        pSymbol->pName = strcpy((char*)STDMALLOC(strlen(pName) + 1), pName);
        if ( pTable->pHashtbl )
        {
            stdHashtbl_Add(pTable->pHashtbl, pSymbol->pName, pSymbol);
        }
    }

    pSymbol->label = sithCogParse_GetNextLabel();
    pSymbol->id    = pSymbol - pTable->aSymbols + pTable->firstId;
    return pSymbol;
}

void J3DAPI sithCogParse_SetSymbolValue(SithCogSymbol* pSymbol, const SithCogSymbolValue* pValue)
{
#line 624
    SITH_ASSERTREL(pSymbol != NULL);
    SITH_ASSERTREL(pValue != NULL);
    memcpy(&pSymbol->value, pValue, sizeof(pSymbol->value));
}

SithCogSymbol* J3DAPI sithCogParse_GetSymbol(const SithCogSymbolTable* pLocal, const char* pName)
{
    SithCogSymbol* pSymbol = NULL;

#line 655
    SITH_ASSERTREL(pLocal != NULL);
    SITH_ASSERTREL(pLocal->aSymbols != NULL);
    SITH_ASSERTREL(pName != NULL);

    if ( pLocal->pHashtbl )
    {
        pSymbol = (SithCogSymbol*)stdHashtbl_Find(pLocal->pHashtbl, pName);
        if ( pSymbol )
        {
            return pSymbol;
        }
    }

    if ( sithCog_pSymbolTable && pLocal != sithCog_pSymbolTable && sithCog_pSymbolTable->pHashtbl )
    {
        pSymbol = (SithCogSymbol*)stdHashtbl_Find(sithCog_pSymbolTable->pHashtbl, pName);
        if ( pSymbol )
        {
            return pSymbol;
        }
    }

    return NULL;
}

SithCogSymbol* J3DAPI sithCogParse_GetSymbolByID(const SithCogSymbolTable* pTable, size_t symbolId)
{
    if ( symbolId >= SITHCOG_GLOBALSYMBOLSTARTID )
    {
        pTable = sithCog_pSymbolTable;
        symbolId -= SITHCOG_GLOBALSYMBOLSTARTID;
    }

    if ( pTable && symbolId < pTable->numUsedSymbols )
    {
        return &pTable->aSymbols[symbolId];
    }

    return NULL;
}

uint32_t J3DAPI sithCogParse_GetSymbolLabel(size_t symbolId)
{
    SithCogSymbol* pSymbol = sithCogParse_GetSymbolByID(pCurrentSymbols, symbolId);
#line 708
    SITH_ASSERTREL(pSymbol);
    return pSymbol->label;
}

SithCogSyntaxNode* J3DAPI sithCogParse_MakeLeafNode(SithCogExecOpcode opcode, int symbolId)
{
    SithCogSyntaxNode* pNewNode = sithCogParse_GetNextNode();
    if ( pNewNode )
    {
        pNewNode->opcode = opcode;
        pNewNode->value  = symbolId;
        pCurrentRoot     = pNewNode;
    }

    return pNewNode;
}

SithCogSyntaxNode* J3DAPI sithCogParse_MakeVectorLeafNode(SithCogExecOpcode opcode, const rdVector3* pVect)
{
    SithCogSyntaxNode* pNewNode = sithCogParse_GetNextNode();
    if ( pNewNode )
    {
        pNewNode->opcode = opcode;
        rdVector_Copy3(&pNewNode->vecValue, pVect);
        pCurrentRoot = pNewNode;
    }

    return pNewNode;
}

SithCogSyntaxNode* J3DAPI sithCogParse_MakeNode(SithCogSyntaxNode* pLeft, SithCogSyntaxNode* pRight, SithCogExecOpcode opcode, int value)
{
    SithCogSyntaxNode* pNewNode = sithCogParse_GetNextNode();
    if ( pNewNode )
    {
        pNewNode->opcode = opcode;
        pNewNode->value  = value;
        pNewNode->pLeft  = pLeft;
        pNewNode->pRight = pRight;
        pCurrentRoot     = pNewNode;
    }

    return pNewNode;
}

void J3DAPI sithCogParse_LexerSetSymbol(const char* pName)
{
    SithCogSymbol* pSymbol;
    SithCogSymbolValue value;

#line 809
    SITH_ASSERTREL(pName && pCurrentSymbols);
    _strlwr((char*)pName);

    pSymbol = sithCogParse_GetSymbol(pCurrentSymbols, pName);
    if ( pSymbol )
    {
        yylval.intValue = pSymbol->id;
        return;
    }

    pSymbol = sithCogParse_AddSymbol(pCurrentSymbols, pName);
    if ( !pSymbol )
    {
#line 833
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "symbol %s not defined, cannot add new symbol.\n", pName, 0, 0, 0);
        return;
    }

#line 839
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parsing script %s: Symbol %s not defined.  Assuming float=0.0\n", pCurrentScript->aName, pName, 0, 0);
    stdPrintf(sith_pHS->pDebugPrint, J3D_FILE, __LINE__, "Parsing script %s: Symbol %s not defined.  Assuming float=0.0\n", pCurrentScript->aName, pName, 0, 0);
    fprintf(stdout, "Parsing script %s: Symbol %s not defined.  Assuming float=0.0\n", pCurrentScript->aName, pName);

    value.type = SITHCOG_VALUE_FLOAT;
    value.val.intValue = 0;
    sithCogParse_SetSymbolValue(pSymbol, &value);

    yylval.intValue = pSymbol->id;
}

void J3DAPI sithCogParse_LexerSetString(const char* pString)
{
    SithCogSymbol* pSymbol;

#line 884
    SITH_ASSERTREL(pString && pCurrentSymbols);

    pSymbol = sithCogParse_AddSymbol(pCurrentSymbols, NULL);
    if ( !pSymbol )
    {
        return;
    }

    pSymbol->value.type = SITHCOG_VALUE_STRING;
#line 897
    pSymbol->value.val.pString = (char*)STDMALLOC(strlen(pString) - 1);
    if ( pSymbol->value.val.pString )
    {
        // Strip the quotes
        strncpy(pSymbol->value.val.pString, pString + 1, strlen(pString) - 2);
        pSymbol->value.val.pString[strlen(pString) - 2] = 0;
    }

    yylval.intValue = pSymbol->id;
}

void J3DAPI sithCogParse_LexerSetVector(const char* pString)
{
    rdVector3 vec;

#line 911
    SITH_ASSERTREL(pString);
    sscanf(pString, "'%f %f %f'", &vec.x, &vec.y, &vec.z);
    rdVector_Copy3(&yylval.vecValue, &vec);
}

uint32_t J3DAPI sithCogParse_GetNextLabel(void)
{
#line 936
    SITH_ASSERTREL(nextlabel <= 1024);
    return nextlabel++;
}

SithCogSyntaxNode* J3DAPI sithCogParse_GetNextNode(void)
{
    SithCogSyntaxNode* pNode;

    if ( !pSyntaxTree )
    {
#line 1027
        pSyntaxTree = (SithCogSyntaxNode*)STDMALLOC(SITHCOGPARSE_SYNTAXTREESIZE * sizeof(SithCogSyntaxNode));
        if ( !pSyntaxTree )
        {
            return NULL;
        }

        syntaxTreeSize = SITHCOGPARSE_SYNTAXTREESIZE;
    }

    if ( numTreeNodes == syntaxTreeSize )
    {
#line 1034
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Max # of nodes %d exceeded, expanding size to %d.\n", syntaxTreeSize, 2 * syntaxTreeSize, 0, 0);
        pSyntaxTree = (SithCogSyntaxNode*)STDREALLOC(pSyntaxTree, 2 * syntaxTreeSize * sizeof(SithCogSyntaxNode));
        if ( !pSyntaxTree )
        {
            return NULL;
        }

        syntaxTreeSize = 2 * syntaxTreeSize;
    }

    pNode = &pSyntaxTree[numTreeNodes++];
    memset(pNode, 0, sizeof(SithCogSyntaxNode));
    return pNode;
}

void J3DAPI sithCogParse_ResetTreeNodes(SithCogSyntaxNode* pRoot)
{
    numTreeNodes = 0;
}

void J3DAPI sithCogParse_GenerateLabelTable(const SithCogSyntaxNode* pRoot)
{
    if ( pRoot->parentLabel )
    {
        aLabelTable[pRoot->parentLabel] = codepos;
    }

    if ( pRoot->pLeft )
    {
        sithCogParse_GenerateLabelTable(pRoot->pLeft);
    }

    if ( pRoot->pRight )
    {
        sithCogParse_GenerateLabelTable(pRoot->pRight);
    }

    switch ( pRoot->opcode )
    {
        case SITHCOGEXEC_OPCODE_PUSHINT:
        case SITHCOGEXEC_OPCODE_PUSHFLOAT:
        case SITHCOGEXEC_OPCODE_PUSHSYMBOL:
        case SITHCOGEXEC_OPCODE_JZ:
        case SITHCOGEXEC_OPCODE_JNZ:
        case SITHCOGEXEC_OPCODE_JMP:
        case SITHCOGEXEC_OPCODE_CALL:
            codepos += 2; // opcode + value
            break;

        case SITHCOGEXEC_OPCODE_PUSHVECTOR:
            codepos += 4; // opcode + vector
            break;

        case SITHCOGEXEC_OPCODE_NOP:
            break;

        default:
            ++codepos;
            break;
    }

    if ( pRoot->childLabel )
    {
        aLabelTable[pRoot->childLabel] = codepos;
    }
}

void J3DAPI sithCogParse_GenerateCode(const SithCogSyntaxNode* pNode)
{
    if ( pNode->pLeft )
    {
        sithCogParse_GenerateCode(pNode->pLeft);
    }

    if ( pNode->pRight )
    {
        sithCogParse_GenerateCode(pNode->pRight);
    }

    if ( pNode->opcode )
    {
        pCurrentCode[codepos++] = pNode->opcode;
        switch ( pNode->opcode )
        {
            case SITHCOGEXEC_OPCODE_PUSHINT:
            case SITHCOGEXEC_OPCODE_PUSHFLOAT:
            case SITHCOGEXEC_OPCODE_PUSHSYMBOL:
                pCurrentCode[codepos++] = pNode->value;
                break;

            case SITHCOGEXEC_OPCODE_PUSHVECTOR:
                rdVector_Copy3((rdVector3*)&pCurrentCode[codepos], &pNode->vecValue);
                codepos += 3;
                break;

            case SITHCOGEXEC_OPCODE_JZ:
            case SITHCOGEXEC_OPCODE_JNZ:
            case SITHCOGEXEC_OPCODE_JMP:
            case SITHCOGEXEC_OPCODE_CALL:
                pCurrentCode[codepos++] = aLabelTable[pNode->value];
                break;

            default:
                return;
        }
    }
}

int J3DAPI sithCogParse_ParseSymbols(SithCogScript* pScript, int bParseDescription)
{
    SithWorld* pWorld;
    int bResult;

    pWorld = sithWorld_pCurrentWorld;
    while ( stdConffile_ReadArgs() && strcmp(stdConffile_entry.aArgs[0].argValue, "end") )
    {
        if ( pScript->pSymbolTable->numUsedSymbols >= pScript->pSymbolTable->tableSize )
        {
#line 1196
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Tried to define too many symbols (%d allocated)\n", pScript->pSymbolTable->tableSize, 0, 0, 0);
            continue;
        }

        if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "thing") )
        {
            bResult = sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_THING, bParseDescription);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "surface") )
        {
            bResult = sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_SURFACE, bParseDescription);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "sector") )
        {
            bResult = sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_SECTOR, bParseDescription);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "sound") )
        {
            bResult = sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_SOUND, bParseDescription);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "template") )
        {
            bResult = sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_TEMPLATE, bParseDescription);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "model") )
        {
            bResult = sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_MODEL, bParseDescription);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "keyframe") )
        {
            bResult = sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_KEYFRAME, bParseDescription);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "cog") )
        {
            bResult = sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_COG, bParseDescription);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "message") )
        {
            bResult = sithCogParse_ParseMessage(pScript);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "material") )
        {
            bResult = sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_MATERIAL, bParseDescription);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "flex") || !strcmp(stdConffile_entry.aArgs[0].argValue, "float") )
        {
            bResult = sithCogParse_ParseFlex(pScript, bParseDescription);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "int") )
        {
            bResult = sithCogParse_ParseInt(pScript, bParseDescription);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "vector") )
        {
            bResult = sithCogParse_ParseVector(pScript, bParseDescription);
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "ai") )
        {
            bResult = sithCogParse_ParseSymbolRef(pScript, SITHCOG_SYM_REF_AICLASS, bParseDescription);
        }
        else
        {
#line 1265
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Symbol type %s not valid.  Symbol %s ignored.\n", pScript->aName, stdConffile_entry.aArgs[0].argValue, stdConffile_entry.aArgs[1].argValue, 0);
            bResult = 0;
        }
    }

    return 1;
}

int J3DAPI sithCogParse_ParseSymbolRef(SithCogScript* pScript, SithCogSymbolRefType symbolType, int bParseDescription)
{
    SithCogSymbolValue curvalue;
    SithCogSymbol* pSymbol;
    SithCogSymbolRef* pRef = NULL;
    size_t i;

    if ( pScript->numSymbolRefs >= SITHCOGSCRIPT_MAXSYMREFS )
    {
#line 1300
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Script %s: Max references exceeded.\n", pScript->aName, 0, 0, 0);
        return 0;
    }

    if ( stdConffile_entry.numArgs < 2 )
    {
#line 1306
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Script %s: Syntax error in symbol definition, line %d.\n", pScript->aName, stdConffile_GetLineNumber(), 0, 0);
        return 0;
    }

    pSymbol = sithCogParse_AddSymbol(pScript->pSymbolTable, stdConffile_entry.aArgs[1].argName);
    if ( !pSymbol )
    {
#line 1313
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Script %s: Could not add new symbol, line %d.\n", pScript->aName, stdConffile_GetLineNumber(), 0, 0);
        return 0;
    }

    curvalue.type = SITHCOG_VALUE_INT;
    curvalue.val.intValue = 0;
    sithCogParse_SetSymbolValue(pSymbol, &curvalue);

    pRef = &pScript->aSymRefs[pScript->numSymbolRefs];
    memset(pRef, 0, sizeof(SithCogSymbolRef));

    pRef->type     = symbolType;
    pRef->mask     = SITHTHING_TYPE2MASK(SITH_THING_FREE) | SITHTHING_TYPE2MASK(SITH_THING_PLAYER);
    pRef->symbolId = pSymbol - pScript->pSymbolTable->aSymbols;

    for ( i = 2; i < stdConffile_entry.numArgs; ++i )
    {
        if ( strcmp(stdConffile_entry.aArgs[i].argName, "local") == 0 )
        {
            pRef->bLocal |= 1;
        }
        else if ( bParseDescription && strcmp(stdConffile_entry.aArgs[i].argName, "desc") == 0 )
        {
            if ( pRef->pDescription )
            {
                stdMemory_Free(pRef->pDescription);
            }

#line 1348
            pRef->pDescription = strcpy((char*)STDMALLOC(strlen(stdConffile_entry.aArgs[i].argValue) + 1), stdConffile_entry.aArgs[i].argValue);
        }
        else if ( strcmp(stdConffile_entry.aArgs[i].argName, "mask") == 0 )
        {
            sscanf(stdConffile_entry.aArgs[i].argValue, "%x", &pRef->mask);
        }
        else if ( strcmp(stdConffile_entry.aArgs[i].argName, "linkid") == 0 )
        {
            pRef->linkId = atoi(stdConffile_entry.aArgs[i].argValue);
        }
        else if ( strcmp(stdConffile_entry.aArgs[i].argName, "nolink") == 0 )
        {
            pRef->linkId = -1;
        }
    }

    if ( stdConffile_entry.aArgs[1].argValue && stdConffile_entry.aArgs[1].argValue != stdConffile_entry.aArgs[1].argName )
    {
        strncpy(pScript->aSymRefs[pScript->numSymbolRefs].aValue, stdConffile_entry.aArgs[1].argValue, STD_ARRAYLEN(pRef->aValue) - 1);
        pScript->aSymRefs[pScript->numSymbolRefs].aValue[STD_ARRAYLEN(pRef->aValue) - 1] = 0;
    }

    ++pScript->numSymbolRefs;
    return 1;
}

int J3DAPI sithCogParse_ParseFlex(SithCogScript* pScript, int bParseDescription)
{
    SithCogSymbol* pSymbol;
    SithCogSymbolValue curvalue;
    char* pDesc;
    size_t idx;
    size_t i;

    pDesc   = NULL;
    pSymbol = sithCogParse_AddSymbol(pScript->pSymbolTable, stdConffile_entry.aArgs[1].argName);
    if ( !pSymbol )
    {
#line 1388
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not add new symbol to Cog, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 0;
    }

    curvalue.type = SITHCOG_VALUE_FLOAT;
    curvalue.val.floatValue = D3DVAL(atof(stdConffile_entry.aArgs[1].argValue));
    sithCogParse_SetSymbolValue(pSymbol, &curvalue);

    for ( i = 2; i < stdConffile_entry.numArgs; ++i )
    {
        if ( strcmp(stdConffile_entry.aArgs[i].argName, "local") == 0 )
        {
            return 1;
        }

        if ( bParseDescription && strcmp(stdConffile_entry.aArgs[i].argName, "desc") == 0 )
        {
#line 1406
            pDesc = strcpy((char*)STDMALLOC(strlen(stdConffile_entry.aArgs[i].argValue) + 1), stdConffile_entry.aArgs[i].argValue);
        }
    }

    idx = pScript->numSymbolRefs;
    pScript->aSymRefs[idx].type         = SITHCOG_SYM_REF_FLEX;
    pScript->aSymRefs[idx].linkId       = -1;
    pScript->aSymRefs[idx].symbolId     = pSymbol->id;
    pScript->aSymRefs[idx].pDescription = pDesc;
    pScript->numSymbolRefs++;
    return 1;
}

int J3DAPI sithCogParse_ParseInt(SithCogScript* pScript, int bParseDescription)
{
    SithCogSymbol* pSymbol;
    SithCogSymbolValue curvalue;
    char* pDesc;
    size_t idx;
    size_t i;

    pDesc   = NULL;
    pSymbol = sithCogParse_AddSymbol(pScript->pSymbolTable, stdConffile_entry.aArgs[1].argName);
    if ( !pSymbol )
    {
#line 1435
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not add new symbol to Cog, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 0;
    }

    curvalue.type = SITHCOG_VALUE_INT;
    curvalue.val.intValue = atoi(stdConffile_entry.aArgs[1].argValue);
    sithCogParse_SetSymbolValue(pSymbol, &curvalue);

    for ( i = 2; i < stdConffile_entry.numArgs; ++i )
    {
        if ( strcmp(stdConffile_entry.aArgs[i].argName, "local") == 0 )
        {
            return 1;
        }

        if ( bParseDescription && strcmp(stdConffile_entry.aArgs[i].argName, "desc") == 0 )
        {
#line 1454
            pDesc = strcpy((char*)STDMALLOC(strlen(stdConffile_entry.aArgs[i].argValue) + 1), stdConffile_entry.aArgs[i].argValue);
        }
    }

    idx = pScript->numSymbolRefs;
    pScript->aSymRefs[idx].type         = SITHCOG_SYM_REF_INT;
    pScript->aSymRefs[idx].linkId       = -1;
    pScript->aSymRefs[idx].symbolId     = pSymbol - pScript->pSymbolTable->aSymbols;
    pScript->aSymRefs[idx].pDescription = pDesc;
    pScript->numSymbolRefs++;
    return 1;
}

int J3DAPI sithCogParse_ParseVector(SithCogScript* pScript, int bParseDescription)
{
    SithCogSymbol* pSymbol;
    SithCogSymbolValue curvalue;
    char* pDesc;
    size_t idx;
    size_t i;

    pDesc   = NULL;
    pSymbol = sithCogParse_AddSymbol(pScript->pSymbolTable, stdConffile_entry.aArgs[1].argName);
    if ( !pSymbol )
    {
#line 1492
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not add new symbol to Cog, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 0;
    }

    curvalue.type = SITHCOG_VALUE_VECTOR;
    curvalue.val.vecValue.x = 0.0f;
    curvalue.val.vecValue.y = 0.0f;
    curvalue.val.vecValue.z = 0.0f;
    sithCogParse_SetSymbolValue(pSymbol, &curvalue);

    for ( i = 2; i < stdConffile_entry.numArgs; ++i )
    {
        if ( strcmp(stdConffile_entry.aArgs[i].argName, "local") == 0 )
        {
            return 1;
        }

        if ( bParseDescription && strcmp(stdConffile_entry.aArgs[i].argName, "desc") == 0 )
        {
#line 1513
            pDesc = strcpy((char*)STDMALLOC(strlen(stdConffile_entry.aArgs[i].argValue) + 1), stdConffile_entry.aArgs[i].argValue);
        }
    }

    idx = pScript->numSymbolRefs;
    pScript->aSymRefs[idx].type         = SITHCOG_SYM_REF_VECTOR;
    pScript->aSymRefs[idx].linkId       = -1;
    pScript->aSymRefs[idx].symbolId     = pSymbol - pScript->pSymbolTable->aSymbols;
    pScript->aSymRefs[idx].pDescription = pDesc;
    pScript->numSymbolRefs++;
    return 1;
}

int J3DAPI sithCogParse_ParseMessage(SithCogScript* pScript)
{
    SithCogSymbol* pSymbol;
    int msgType;

    if ( pScript->numHandlers == STD_ARRAYLEN(pScript->aHandlers) )
    {
#line 1554
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many message handlers in script %s.\n", pScript->aName, 0, 0, 0);
        return 0;
    }

    pSymbol = sithCogParse_GetSymbol(sithCog_pSymbolTable, stdConffile_entry.aArgs[1].argValue);
    if ( !pSymbol )
    {
#line 1563
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog message type %s is not recognized, %s.\n", stdConffile_entry.aArgs[1].argValue, pScript->aName, 0, 0);
        return 0;
    }

    msgType = pSymbol->value.val.intValue;
    pSymbol = sithCogParse_AddSymbol(pScript->pSymbolTable, stdConffile_entry.aArgs[1].argValue);
    if ( !pSymbol )
    {
        return 0;
    }

    pSymbol->value.type         = SITHCOG_VALUE_INT;
    pSymbol->value.val.intValue = msgType;

    pScript->aHandlers[pScript->numHandlers].type  = msgType;
    pScript->aHandlers[pScript->numHandlers].label = pSymbol->label;
    ++pScript->numHandlers;
    return 1;
}

void J3DAPI sithCogParse_ResolveMessageHandlers(SithCogScript* pScript)
{
    size_t i;
    for ( i = 0; i < pScript->numHandlers; ++i )
    {
        pScript->aHandlers[i].codeOffset = aLabelTable[pScript->aHandlers[i].label];
    }
}
