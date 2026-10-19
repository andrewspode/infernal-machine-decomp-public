#include "wuRegistry.h"
#include <std/General/std.h>

static int wuRegistry_bStarted = 0;
static LPCSTR appSubKey = NULL;
static HKEY appRootKey  = NULL;

int J3DAPI wuRegistry_Startup(HKEY rootKey, LPCSTR subKey)
{
    HKEY hkResult = 0;
    DWORD dwDisposition;
    LONG status;

#line 62
    STD_ASSERTREL(!wuRegistry_bStarted);

    // Retail key set
#line 63
    STD_ASSERTREL(
        rootKey == HKEY_CLASSES_ROOT || rootKey == HKEY_CURRENT_USER
     || rootKey == HKEY_LOCAL_MACHINE || rootKey == HKEY_USERS
     || rootKey == HKEY_PERFORMANCE_DATA
     || rootKey == HKEY_CURRENT_CONFIG
     || rootKey == HKEY_DYN_DATA
    );

    wuRegistry_bStarted = 1;
    appSubKey  = subKey;
    appRootKey = rootKey;

    if ( (status = RegCreateKeyEx(
              appRootKey,
              appSubKey,
              0,
              "",
              REG_OPTION_NON_VOLATILE,
              KEY_WRITE | KEY_READ, // 0x2001F -  KEY_WRITE | KEY_READ
              NULL,
              &hkResult,
              &dwDisposition)) )
    {
        goto error;
    }

    if ( (status = RegCloseKey(hkResult)) )
    {
        goto error;
    }

    hkResult = 0;
    return 0;

error:
    if ( hkResult )
    {
        RegCloseKey(hkResult);
    }

    return 1;
}

void J3DAPI wuRegistry_Shutdown(void)
{
    appSubKey           = NULL;
    appRootKey          = NULL;
    wuRegistry_bStarted = 0;
}

int J3DAPI wuRegistry_SaveInt(const char* pKey, int value)
{
    HKEY hkResult = 0;
    LONG status;

#line 138
    STD_ASSERTREL(wuRegistry_bStarted);

    // Retail key set
#line 139
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT || appRootKey == HKEY_CURRENT_USER
     || appRootKey == HKEY_LOCAL_MACHINE || appRootKey == HKEY_USERS
     || appRootKey == HKEY_PERFORMANCE_DATA
     || appRootKey == HKEY_CURRENT_CONFIG
     || appRootKey == HKEY_DYN_DATA
    );

    if ( (status = RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult)) )
    {
        goto error;
    }

    if ( (status = RegSetValueEx(hkResult, pKey, 0, REG_DWORD, (const BYTE*)&value, sizeof(value))) )
    {
        goto error;
    }

    if ( (status = RegCloseKey(hkResult)) )
    {
        goto error;
    }

    hkResult = 0;
    return 0;

error:
    if ( hkResult )
    {
        RegCloseKey(hkResult);
    }

    return 1;
}

int J3DAPI wuRegistry_SaveFloat(const char* pKey, float value)
{
    HKEY hkResult = 0;
    LONG status;

#line 186
    STD_ASSERTREL(wuRegistry_bStarted);

    // Retail key set
#line 187
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT || appRootKey == HKEY_CURRENT_USER
     || appRootKey == HKEY_LOCAL_MACHINE || appRootKey == HKEY_USERS
     || appRootKey == HKEY_PERFORMANCE_DATA
     || appRootKey == HKEY_CURRENT_CONFIG
     || appRootKey == HKEY_DYN_DATA
    );

    if ( (status = RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult)) )
    {
        goto error;
    }

    if ( (status = RegSetValueEx(hkResult, pKey, 0, REG_BINARY, (const BYTE*)&value, sizeof(value))) )
    {
        goto error;
    }

    if ( (status = RegCloseKey(hkResult)) )
    {
        goto error;
    }

    hkResult = 0;
    return 0;

error:
    if ( hkResult )
    {
        RegCloseKey(hkResult);
    }

    return 1;
}

int J3DAPI wuRegistry_GetInt(const char* pKey, int defaultValue)
{
    HKEY hkRet = 0;
    BYTE data[sizeof(int)];
    DWORD lengthInfo;

#line 235
    STD_ASSERTREL(wuRegistry_bStarted);

    // Retail key set
#line 236
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT || appRootKey == HKEY_CURRENT_USER
     || appRootKey == HKEY_LOCAL_MACHINE || appRootKey == HKEY_USERS
     || appRootKey == HKEY_PERFORMANCE_DATA
     || appRootKey == HKEY_CURRENT_CONFIG
     || appRootKey == HKEY_DYN_DATA
    );

    if ( RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkRet) == ERROR_SUCCESS )
    {
        DWORD type;

        lengthInfo = sizeof(data);
        if ( RegQueryValueEx(hkRet, pKey, NULL, &type, data, &lengthInfo) == ERROR_SUCCESS )
        {
            RegCloseKey(hkRet);
            return *(uint32_t*)data;
        }

        RegCloseKey(hkRet);
    }

    return defaultValue;
}

float J3DAPI wuRegistry_GetFloat(const char* pKey, float defaultValue)
{
    HKEY hkRet;
    BYTE data[sizeof(int)];
    DWORD lengthInfo;

#line 282
    STD_ASSERTREL(wuRegistry_bStarted);

    // Retail key set
#line 283
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT || appRootKey == HKEY_CURRENT_USER
     || appRootKey == HKEY_LOCAL_MACHINE || appRootKey == HKEY_USERS
     || appRootKey == HKEY_PERFORMANCE_DATA
     || appRootKey == HKEY_CURRENT_CONFIG
     || appRootKey == HKEY_DYN_DATA
    );

    if ( RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkRet) == ERROR_SUCCESS )
    {
        DWORD type;

        lengthInfo = sizeof(data);
        if ( RegQueryValueExA(hkRet, pKey, 0, &type, data, &lengthInfo) == ERROR_SUCCESS )
        {
            RegCloseKey(hkRet);
            return *(float*)data;
        }

        RegCloseKey(hkRet);
    }

    return defaultValue;
}

int J3DAPI wuRegistry_SaveIntEx(const char* pKey, int defaultValue)
{
#line 325
    STD_ASSERTREL(wuRegistry_bStarted);
    return wuRegistry_SaveInt(pKey, defaultValue);
}

int J3DAPI wuRegistry_GetIntEx(const char* pKey, int defaultValue)
{
#line 344
    STD_ASSERTREL(wuRegistry_bStarted);
    return wuRegistry_GetInt(pKey, defaultValue);
}

/* SaveBinary/GetBinary: discarded by the retail linker (no caller in either image), restored in full from
   the debug build (0x00503B13, 0x00503BFC; __LINE__ 364/371, 406/413). Names from OpenJones3D develop. */
int J3DAPI wuRegistry_SaveBinary(const char* pKey, const uint8_t* pData, size_t size)
{
    HKEY hkResult;

#line 364
    STD_ASSERTREL(wuRegistry_bStarted);

#line 371
    STD_ASSERTREL(appRootKey == HKEY_CLASSES_ROOT || appRootKey == HKEY_CURRENT_USER || appRootKey == HKEY_LOCAL_MACHINE || appRootKey == HKEY_USERS || appRootKey == HKEY_PERFORMANCE_DATA || appRootKey == HKEY_CURRENT_CONFIG || appRootKey == HKEY_DYN_DATA);

    if ( !RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult) )
    {
        RegSetValueEx(hkResult, pKey, 0, REG_BINARY, pData, size);
        RegCloseKey(hkResult);
        return 1;
    }

    return 0;
}

int J3DAPI wuRegistry_GetBinary(const char* pKey, uint8_t* pData, size_t size)
{
    HKEY hkResult;
    DWORD valueType;

#line 406
    STD_ASSERTREL(wuRegistry_bStarted);

#line 413
    STD_ASSERTREL(appRootKey == HKEY_CLASSES_ROOT || appRootKey == HKEY_CURRENT_USER || appRootKey == HKEY_LOCAL_MACHINE || appRootKey == HKEY_USERS || appRootKey == HKEY_PERFORMANCE_DATA || appRootKey == HKEY_CURRENT_CONFIG || appRootKey == HKEY_DYN_DATA);

    if ( !RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult) )
    {
        if ( !RegQueryValueEx(hkResult, pKey, NULL, &valueType, pData, &size) )
        {
            RegCloseKey(hkResult);
            return 1;
        }

        RegCloseKey(hkResult);
    }

    return 0;
}

int J3DAPI wuRegistry_SaveStr(const char* pKey, const char* pStr)
{
    HKEY hkResult = 0;
    LONG status;

#line 451
    STD_ASSERTREL(wuRegistry_bStarted);

    // Retail key set
#line 452
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT || appRootKey == HKEY_CURRENT_USER
     || appRootKey == HKEY_LOCAL_MACHINE || appRootKey == HKEY_USERS
     || appRootKey == HKEY_PERFORMANCE_DATA
     || appRootKey == HKEY_CURRENT_CONFIG
     || appRootKey == HKEY_DYN_DATA
    );

    if ( (status = RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult)) )
    {
        goto error;
    }

    if ( (status = RegSetValueEx(hkResult, pKey, 0, REG_SZ, (const uint8_t*)pStr, strlen(pStr))) )
    {
        goto error;
    }

    if ( (status = RegCloseKey(hkResult)) )
    {
        goto error;
    }

    hkResult = 0;
    return 0;

error:
    if ( hkResult )
    {
        RegCloseKey(hkResult);
    }

    return 1;
}

int J3DAPI wuRegistry_GetStr(const char* pKey, char* pDstStr, size_t size, const char* pDefaultValue)
{
    HKEY hkResult = 0;
    DWORD curtype;
    DWORD dataSize;
    LONG status;

#line 506
    STD_ASSERTREL(wuRegistry_bStarted);

    // Retail key set
#line 507
    STD_ASSERTREL(
        appRootKey == HKEY_CLASSES_ROOT || appRootKey == HKEY_CURRENT_USER
     || appRootKey == HKEY_LOCAL_MACHINE || appRootKey == HKEY_USERS
     || appRootKey == HKEY_PERFORMANCE_DATA
     || appRootKey == HKEY_CURRENT_CONFIG
     || appRootKey == HKEY_DYN_DATA
    );

    if ( (status = RegOpenKeyEx(appRootKey, appSubKey, 0, KEY_ALL_ACCESS, &hkResult)) )
    {
        goto error;
    }

    dataSize = size;
    if ( (status = RegQueryValueEx(hkResult, pKey, NULL, &curtype, (LPBYTE)pDstStr, &dataSize)) )
    {
        goto error;
    }

    if ( (status = RegCloseKey(hkResult)) )
    {
        goto error;
    }

    hkResult = 0;
    return 0;

error:
    if ( !pDefaultValue )
    {
        pDstStr = NULL;
    }
    else
    {
        strncpy(pDstStr, pDefaultValue, size - 1);
        pDstStr[size - 1] = '\0';
    }

    if ( hkResult )
    {
        RegCloseKey(hkResult);
    }

    return 1;
}
