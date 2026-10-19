#include "stdUtil.h"

#include <ctype.h>

int J3DAPI stdUtil_StrCmp(const char* str1, const char* str2)
{
    char c1;
    char c2;
    while ( 1 )
    {
        c1 = (char)tolower(*str1);
        c2 = (char)tolower(*str2);
        if ( !c1 && !c2 ) return 0;
        if ( c2 > c1 ) return -1;
        if ( c2 < c1 ) return 1;
        ++str1;
        ++str2;
    }
}
