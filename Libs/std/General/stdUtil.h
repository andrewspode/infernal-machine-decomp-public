#ifndef STD_STDUTIL_H
#define STD_STDUTIL_H
#include <string.h>
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

/**
* @brief Returns the number of elements in a static array.
* @param x - The array whose length is to be calculated.
*/
#define STD_ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

/**
 * @brief Converts wide string to string.
 *
 * Ensures `dest` is an char array and calls `stdUtil_ToAStringEx`.
 *
 * @param dest - Destination char array buffer.
 * @param src - Source wide string.
*/
#define STD_TOSTR(dest, src)  \
    {\
        stdUtil_ToAStringEx(dest, src, STD_ARRAYLEN(dest) - 1); \
        (dest)[STD_ARRAYLEN(dest) - 1] = 0; \
    }


/**
 * @brief Converts string to wide string.
 *
 * Ensures `dest` is an wchar_t array and calls `stdUtil_ToWStringEx`.
 *
 * @param dest - Destination whcar_t array buffer.
 * @param src - Source string.
*/
#define STD_TOWSTR(dest, src)  \
    {\
        stdUtil_ToWStringEx(dest, src, STD_ARRAYLEN(dest) - 1); \
        (dest)[STD_ARRAYLEN(dest) - 1] = 0; \
    }


char* J3DAPI stdUtil_FastCopy(const char* pSource, tHostServices* pHS);
void J3DAPI stdUtil_Format(char* pStr, size_t size, const char* format, ...);
char* J3DAPI stdUtil_StringSplit(const char* pSource, char* pFirstToken, size_t maxTokenLenght, const char* pSeparators); // Splits string on the first occurrence of pSeparators
char* J3DAPI stdUtil_ParseLiteral(const char* pSource, char* pDest, size_t destSize);

wchar_t* J3DAPI stdUtil_ToWString(const char* pString);
int J3DAPI stdUtil_ToWStringEx(wchar_t* pwString, const char* pString, int maxChars);

char* J3DAPI stdUtil_ToAString(const wchar_t* pwString);
int J3DAPI stdUtil_ToAStringEx(char* pString, const wchar_t* pwString, int maxChars);
void J3DAPI stdUtil_WstrRemoveCharsAt(wchar_t* pwStr, int index, int numToDel);
void J3DAPI stdUtil_wstrncat(wchar_t* pwSource, int sourceSize, int index, wchar_t* pwStrToInsert);

void J3DAPI stdUtil_ToLower(char* pStr);
uint32_t J3DAPI stdUtil_CalcChecksum(const uint8_t* pData, size_t dataSize, uint32_t seed);

// Retail body at 0x004E09A0 is not part of the stdUtil.c object.
int J3DAPI stdUtil_StrCmp(const char* str1, const char* str2);

J3D_EXTERN_C_END
#endif // STD_STDUTIL_H
