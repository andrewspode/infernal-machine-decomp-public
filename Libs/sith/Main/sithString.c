#include "sithMain.h"
#include "sithString.h"


#include <std/General/stdMemory.h>
#include <std/General/stdStrTable.h>

static tStringTable g_sithStringsTbl;
static tStringTable voiceStringsTbl;
static int bStartup = 0;


int J3DAPI sithString_Startup(void)
{
    if ( bStartup )
    {
#line 60
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "sithString startup -- already started!\n", 0, 0, 0, 0);
        return 0;
    }

    if ( !stdStrTable_Load(&voiceStringsTbl, "misc\\voiceStrings.uni")
           && !stdStrTable_Load(&voiceStringsTbl, "\\Jones3D\\resource\\misc\\voiceStrings.uni") )
    {
#line 69
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error opening stringtable 'misc\\voiceStrings.uni'.\n", 0, 0, 0, 0);
        return 1;
    }
    else if ( !stdStrTable_Load(&g_sithStringsTbl, "misc\\sithStrings.uni")
           && !stdStrTable_Load(&g_sithStringsTbl, "\\Jones3D\\resource\\misc\\sithStrings.uni") )
    {
#line 79
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error opening stringtable 'misc\\sithStrings.uni'.\n", 0, 0, 0, 0);
        return 1;
    }

    // Success
    bStartup = 1;
    return 0;
}

void J3DAPI sithString_Shutdown(void)
{
    if ( bStartup )
    {
        stdStrTable_Free(&voiceStringsTbl);
        stdStrTable_Free(&g_sithStringsTbl);
    }

    bStartup = 0;
}

/* Linker-discarded: retail __LINE__ span 79..164 (sithString_Startup -> sithString_GetString) is +28
   lines over the overhead; the debug build has this function (address 0x0044D5CA)
 between Shutdown and GetString (32 lines with overhead). No retail symbol or caller. Only
   existence and position are evidenced; the name and signature are inferred.
   Body restored from the debug build's text. */
wchar_t* J3DAPI sithString_sub_44D5CA(const char* pStr)
{
    wchar_t* pString;

#line 130
    SITH_ASSERTREL(bStartup);
    if ( !pStr )
    {
        return NULL;
    }

    if ( strncmp("SITH", pStr, 4u) == 0 )
    {
        pString = stdStrTable_GetValue(&g_sithStringsTbl, pStr);
    }
    else
    {
        pString = stdStrTable_GetValue(&voiceStringsTbl, pStr);
    }

    return pString;
}

wchar_t* J3DAPI sithString_GetString(const char* pStr)
{
    wchar_t* pString;

#line 164
    SITH_ASSERTREL(bStartup);
    if ( !pStr )
    {
#line 168
        wchar_t* pRetStr = (wchar_t*)STDMALLOC(sizeof(wchar_t));
        memset(pRetStr, 0, sizeof(wchar_t));
        return pRetStr;
    }

    if ( !strncmp("SITH", pStr, 4u) )
    {
        pString = stdStrTable_GetValue(&g_sithStringsTbl, pStr);
    }
    else
    {
        pString = stdStrTable_GetValueOrKey(&voiceStringsTbl, pStr);
    }

    return pString;
}
