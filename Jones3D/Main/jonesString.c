#include "jonesString.h"

#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdStrTable.h>
#include <std/General/stdUtil.h>

static tStringTable jonesString_jonesStringsTbl;
static char jonesString_aBuffer[512] = { 0 };
static int jonesString_bStarted = 0;

/* Linker-discarded (no caller in either image); the debug build keeps it at 0x004261E0, before GetString. Returns an
   allocated ANSI copy of the key's string, or NULL. */
char* J3DAPI jonesString_GetAString(const char* pKey)
{
    wchar_t* pWString = NULL;

#line 40
    STD_ASSERTREL(jonesString_bStarted);
    if ( !pKey )
    {
        return NULL;
    }

    pWString = stdStrTable_GetValue(&jonesString_jonesStringsTbl, pKey);
    if ( pWString )
    {
        return stdUtil_ToAString(pWString);
    }

    return NULL;
}

const char* J3DAPI jonesString_GetString(const char* pKey)
{
    wchar_t* pWString = NULL;
    char* pAString = NULL;

    if ( !jonesString_bStarted ) {
        return NULL;
    }

    if ( !pKey ) {
        return NULL;
    }

    pWString = stdStrTable_GetValueOrKey(&jonesString_jonesStringsTbl, pKey);
    if ( pWString )
    {
        pAString = stdUtil_ToAString(pWString);
        strcpy(jonesString_aBuffer, pAString);
        stdMemory_Free(pAString);
        return jonesString_aBuffer;
    }

    strcpy(jonesString_aBuffer, pKey);
    return jonesString_aBuffer;
}

int J3DAPI jonesString_Startup(void)
{
    if ( jonesString_bStarted ) {
        return 0;
    }

    if ( !stdStrTable_Load(&jonesString_jonesStringsTbl, "misc\\jonesStrings.uni") )
    {
#line 132
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "couldn't find file misc\\jonesStrings.uni\n", 0, 0, 0, 0);
        return 1;
    }

    jonesString_bStarted = 1;
    return 0;
}

void J3DAPI jonesString_Shutdown(void)
{
    if ( !jonesString_bStarted )
    {
        return;
    }

    stdStrTable_Free(&jonesString_jonesStringsTbl);
    jonesString_bStarted = 0;
}
