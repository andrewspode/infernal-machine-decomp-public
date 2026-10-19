/*
 * insane_lecshare: the LEC module framework's share registry (retail 0x004EB210-0x004EBDE0): a registry of up to 512 published
 * interfaces, looked up by `NAME` or `NAME=provider` strings, plus the start-up/shut-down walk over the registered modules and a
 * small formatted-message ring. `lec.h` names its table.
 */
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../lec.h"
#include "LecShare.h"

#define LEC_MAXENTRIES 512

typedef struct LecEntry
{
    const char* pName;
    void* pInterface;
    int refs;
} LecEntry;

static void LecShare_Reset(const char* pA, const char* pB);
static int LecShare_Publish(const char* pName, void* pInterface);
static int LecShare_Unpublish(const char* pName);
static void* LecShare_Subscribe(const char* pName, const char* pPreferred);
static int LecShare_Unsubscribe(void* pInterface);
static int LecShare_BatchSubscribe(LecBatch* pTable, const char* pDefault);
static int LecShare_BatchUnsubscribe(LecBatch* pTable);
static void* LecShare_Enumerate(const char* pName, int index);
static int LecShare_Count(const char* pName);
static LecEntry* LecShare_Find(const char* pName);
static int LecShare_Match(const char* pName, const char* pList);
#define LEC_MATCH(n, l) ((const char*)LecShare_Match(n, l))
static int LecShare_NameLen(const char* pName);
static int LecShare_NullError(const char* pFormat, ...);
static void LecShare_Strip(char* pName);
static int LecShare_NameCmp(const char* pA, const char* pB);
static int LecShare_NameNCmp(const char* pA, const char* pB, int n);
char* lec_GetLog(int bFlush);

static LecShare lec_table = {
    LecShare_Publish, (void (*)(const char*))LecShare_Unpublish, (void* (*)(const char*, void*))LecShare_Subscribe, (void (*)(void*))LecShare_Unsubscribe,
    LecShare_BatchSubscribe, LecShare_BatchUnsubscribe, LecShare_Enumerate, LecShare_Count, (void (*)(const char*, ...))LecShare_NullError
};

static char lec_nameBuf[0x64] = { 0 };
static char lec_parseBuf[0x2000] = { 0 };
static struct
{
    int inited;
    const char* pA;
    const char* pB;
    LecEntry* pCursor;
    int count;
    LecEntry entries[LEC_MAXENTRIES];
    LecEntry* pSearch;
} lec_state = { 0 };
static int lec_enumIndex = 0;
static char lec_ring[20][200] = { 0 };
static int lec_ringNext = 0;
static int lec_ringUsed = 0;

LecShare* LecShare_Init(LecErrorFn pError, LecModuleEntry* pModules, const char* pA, const char* pB)
{
    if (pError) {
        lec_table.ReportError = pError;
    }
    LecShare_Reset(pA, pB);
    lec_state.inited = 0;
    lec_state.pSearch = NULL;
    if (pModules) {
        while (pModules->pfnRegister) {
            pModules->pfnRegister(&lec_table, pModules->pParam);
            pModules++;
        }
    }
    lec_state.inited = 1;
    return &lec_table;
}

int LecShare_UnregisterAll(void)
{
    int n = lec_state.count;
    LecEntry* p = &lec_state.entries[n - 1];

    while (n-- != 0) {
        if (p->pName && LecShare_NameLen(p->pName) == 9 && LecShare_NameNCmp("LECMODULE", p->pName, LecShare_NameLen(p->pName)) == 0) {
            if (((LecModule*)p->pInterface)->Unregister) {
                ((LecModule*)p->pInterface)->Unregister();
            }
        }
        p--;
    }
    return 0;
}

int LecShare_Startup(void)
{
    LecEntry* p = lec_state.entries;
    int n = lec_state.count;
    int rc = 0;

    lec_state.pCursor = NULL;
    while (n-- != 0) {
        if (p->pName && LecShare_NameLen(p->pName) == 9 && LecShare_NameNCmp("LECMODULE", p->pName, LecShare_NameLen(p->pName)) == 0) {
            if (((LecModule*)p->pInterface)->Startup) {
                rc = ((LecModule*)p->pInterface)->Startup();
                if (rc) {
                    if (lec_table.ReportError) {
                        lec_table.ReportError("LecShare_Startup: Failed with module %s.", p->pName);
                    }
                    LecShare_Shutdown();
                    return rc;
                }
            }
            lec_state.pCursor = p;
        }
        p++;
    }
    return rc;
}

int LecShare_Shutdown(void)
{
    LecEntry* p;
    int n;

    if (lec_state.pCursor == NULL) {
        return 1;
    }
    p = ++lec_state.pCursor;
    n = (int)(p - lec_state.entries);
    p--;
    while (n-- != 0) {
        if (p->pName && LecShare_NameLen(p->pName) == 9 && LecShare_NameNCmp("LECMODULE", p->pName, LecShare_NameLen(p->pName)) == 0) {
            if (((LecModule*)p->pInterface)->Shutdown) {
                ((LecModule*)p->pInterface)->Shutdown();
            }
        }
        p--;
    }
    return 0;
}

static void LecShare_Reset(const char* pA, const char* pB)
{
    memset(&lec_state, 0, sizeof(lec_state));
    lec_state.pA = pA;
    lec_state.pB = pB;
}

static int LecShare_Publish(const char* pName, void* pInterface)
{
    int n = lec_state.count;
    LecEntry* p = &lec_state.entries[n - 1];
    int i = n;

    lec_state.pSearch = NULL;
    while (i-- != 0) {
        if (p->pName == NULL) {
            p->pName = pName;
            p->pInterface = pInterface;
            p->refs = 0;
            return 0;
        }
        p--;
    }
    if ((unsigned)n >= LEC_MAXENTRIES) {
        return 1;
    }
    lec_state.count = n + 1;
    lec_state.entries[n].pName = pName;
    lec_state.entries[n].pInterface = pInterface;
    lec_state.entries[n].refs = 0;
    return 0;
}

static int LecShare_Unpublish(const char* pName)
{
    int n = lec_state.count;
    LecEntry* p;

    lec_state.pSearch = NULL;
    LecShare_Strip((char*)pName);
    p = &lec_state.entries[n - 1];
    while (n-- != 0) {
        if (p->pName && LecShare_NameCmp(p->pName, pName) == 0) {
            int refs = p->refs;

            memset(p, 0, sizeof(*p));
            return refs;
        }
        p--;
    }
    return 1;
}

static void* LecShare_Subscribe(const char* pName, const char* pPreferred)
{
    char buf[0x64];
    const char* pProvider = NULL;
    int len;

    if (!lec_state.inited) {
        return NULL;
    }
    lec_state.pSearch = NULL;
    if (lec_state.pB) {
        pProvider = LEC_MATCH(pName, lec_state.pB);
    }
    if (!pProvider) {
        if (pPreferred) {
            pProvider = LEC_MATCH(pName, pPreferred);
        }
        if (!pProvider) {
            if (lec_state.pA) {
                pProvider = LEC_MATCH(pName, lec_state.pA);
            }
        }
    }
    len = LecShare_NameLen(pName);
    strncpy(buf, pName, len);
    buf[len] = 0;
    if (!pProvider) {
        if (!LecShare_Count(buf)) {
            return NULL;
        }
        strncpy(buf, (const char*)LecShare_Enumerate(buf, 0), 99);
    }
    else {
        strcat(buf, "=");
        strcat(buf, pProvider);
    }
    {
        LecEntry* pEntry = LecShare_Find(buf);

        if (pEntry) {
            pEntry->refs++;
            return pEntry->pInterface;
        }
    }
    return NULL;
}

static int LecShare_Unsubscribe(void* pInterface)
{
    int n = lec_state.count;
    LecEntry* p = &lec_state.entries[n - 1];
    int i = n;

    lec_state.pSearch = NULL;
    while (i-- != 0) {
        if (pInterface == p->pInterface) {
            p->refs--;
            return 0;
        }
        p--;
    }
    return 1;
}

static int LecShare_BatchSubscribe(LecBatch* pTable, const char* pDefault)
{
    LecBatch* p;

    for (p = pTable; p->ppOut; p++) {
        if (p->pPreferred && *p->pPreferred) {
            *p->ppOut = LecShare_Subscribe(p->pName, p->pPreferred);
        }
        else {
            *p->ppOut = NULL;
        }
        if (*p->ppOut == NULL && pDefault && *pDefault) {
            *p->ppOut = LecShare_Subscribe(p->pName, pDefault);
        }
        if (*p->ppOut == NULL) {
            *p->ppOut = LecShare_Subscribe(p->pName, NULL);
        }
        if (*p->ppOut == NULL) {
            if (lec_table.ReportError) {
                lec_table.ReportError("LecShare:BatchSubscribe Couldn't subscribe to %s interface.", p->pName);
            }
            return 1;
        }
    }
    return 0;
}

static int LecShare_BatchUnsubscribe(LecBatch* pTable)
{
    LecBatch* p;
    int n;

    if (pTable->ppOut == NULL) {
        return 1;
    }
    p = pTable + 1;
    n = 1;
    while (p->ppOut) {
        p++;
        n++;
    }
    p--;
    while (n-- != 0) {
        if (LecShare_Unsubscribe(*p->ppOut)) {
            if (lec_table.ReportError) {
                lec_table.ReportError("LecShare:BatchUnsubscribe Couldn't unsubscribe from %s interface.", p->pName);
            }
            return 1;
        }
        *p->ppOut = NULL;
        p--;
    }
    return 0;
}

static void* LecShare_Enumerate(const char* pName, int index)
{
    int step = 1;
    int n;
    LecEntry* p;
    int want;

    if (lec_state.count == 0 || index < 0) {
        return NULL;
    }
    n = lec_state.count;
    if (lec_state.pSearch && ((!pName && lec_nameBuf[0] == 0) || (pName && LecShare_NameCmp(pName, lec_nameBuf) == 0))) {
        if (index == lec_enumIndex) {
            return (void*)lec_state.pSearch->pName;
        }
        if (index < lec_enumIndex) {
            step = -1;
            n = (int)(lec_state.pSearch - lec_state.entries);
            want = index;
            index = lec_enumIndex - index - 1;
            lec_enumIndex = want;
        }
        else {
            n -= (int)(lec_state.pSearch - lec_state.entries) + 1;
            want = index;
            index -= lec_enumIndex + 1;
            lec_enumIndex = want;
        }
        p = lec_state.pSearch + step;
    }
    else {
        p = lec_state.entries;
        lec_enumIndex = index;
    }
    if (pName) {
        strncpy(lec_nameBuf, pName, 99);
        lec_nameBuf[99] = 0;
    }
    else {
        lec_nameBuf[0] = 0;
    }
    while (n-- != 0) {
        if (p->pName) {
            if (pName == NULL || ((int)strlen(pName) == LecShare_NameLen(p->pName) && LecShare_NameNCmp(pName, p->pName, LecShare_NameLen(p->pName)) == 0)) {
                if (index-- == 0) {
                    lec_state.pSearch = p;
                    return (void*)p->pName;
                }
            }
        }
        p += step;
    }
    lec_state.pSearch = NULL;
    return NULL;
}

static int LecShare_Count(const char* pName)
{
    int n = 0;
    int i = lec_state.count;
    LecEntry* p = &lec_state.entries[i - 1];

    while (i-- != 0) {
        if (p->pName) {
            if (pName == NULL || ((int)strlen(pName) == LecShare_NameLen(p->pName) && LecShare_NameNCmp(pName, p->pName, LecShare_NameLen(p->pName)) == 0)) {
                n++;
            }
        }
        p--;
    }
    return n;
}

static LecEntry* LecShare_Find(const char* pName)
{
    int n = lec_state.count;
    LecEntry* p = &lec_state.entries[n - 1];
    int i = n;

    while (i-- != 0) {
        if (LecShare_NameCmp(p->pName, pName) == 0) {
            return p;
        }
        p--;
    }
    return NULL;
}

#define LEC_SEP(c) ((c) < 0x21 || (c) > 0x7F || (c) == ',' || (c) == ';')

static int LecShare_Match(const char* pName, const char* pList)
{
    const char* pResult = NULL;
    int found = 0;
    char* p;
    const char* q;
    int atEnd = 0;

    strncpy(lec_parseBuf, pList, strlen(pList) < 0x1FFF ? strlen(pList) : 0x1FFF);
    lec_parseBuf[strlen(pList) < 0x1FFF ? strlen(pList) : 0x1FFF] = 0;
    p = lec_parseBuf;
    while (*p) {
        while (*p && LEC_SEP(*p)) {
            p++;
        }
        q = pName;
        while (*p && *q && toupper(*q) == toupper(*p)) {
            p++;
            q++;
        }
        if (*q == 0 && *p == '.') {
            q = pName;
        }
        atEnd = !(*p && *p >= 0x21 && *p <= 0x7F && *p != '=');
        while (*p && *p != '=') {
            p++;
        }
        if (*p == 0) {
            return (int)pResult;
        }
        p++;
        while (*p && LEC_SEP(*p)) {
            p++;
        }
        if (atEnd && *q == 0) {
            pResult = p;
        }
        else if (*q == '.') {
            pResult = p;
            found = 1;
        }
        while (*p && !LEC_SEP(*p)) {
            p++;
        }
        if (*q == 0 || found) {
            if (*p == 0) {
                return (int)pResult;
            }
            *p++ = 0;
            if (!found) {
                return (int)pResult;
            }
            found = 0;
        }
        while (*p && LEC_SEP(*p)) {
            p++;
        }
    }
    return (int)pResult;
}

static int LecShare_NameLen(const char* pName)
{
    const char* p = pName;

    while (*p) {
        if (*p == '.' || *p == '=') {
            break;
        }
        p++;
    }
    if (p > pName && *p == '=') {
        do {
            p--;
        } while (p > pName && *p && LEC_SEP(*p));
        p++;
    }
    return (int)(p - pName);
}

static int LecShare_NullError(const char* pFormat, ...)
{
    return 0;
}

static void LecShare_Strip(char* pName)
{
    char* pDst = pName;
    char* pSrc = pName;

    if (*pSrc) {
        do {
            if (*pSrc >= 0x21 && *pSrc <= 0x7F) {
                *pDst++ = *pSrc;
            }
        } while (*++pSrc);
    }
    *pDst = 0;
}

static int LecShare_NameCmp(const char* pA, const char* pB)
{
    int r = -1;

    for (;;) {
        if (*pA == 0 && *pB == 0) {
            break;
        }
        r = toupper(*pA++) - toupper(*pB++);
        if (r) {
            break;
        }
    }
    return r;
}

static int LecShare_NameNCmp(const char* pA, const char* pB, int n)
{
    int r = -1;
    const char* a = pA;
    const char* b = pB;

    while (n-- != 0) {
        if (*a == 0 && *b == 0) {
            break;
        }
        r = toupper(*a++) - toupper(*b++);
        if (r) {
            break;
        }
    }
    return r;
}

void lec_Log(const char* pFormat, ...)
{
    va_list args;

    va_start(args, pFormat);
    vsprintf(lec_ring[lec_ringNext], pFormat, args);
    if (lec_ringNext == 19) {
        lec_ringNext = 0;
    }
    else {
        lec_ringNext++;
    }
    if (lec_ringUsed < 19) {
        lec_ringUsed++;
    }
}

char* lec_GetLog(int bFlush)
{
    int i;

    if (lec_ringUsed != 0) {
        i = lec_ringNext - lec_ringUsed;
        while (i < 0) {
            i += 20;
        }
        if (bFlush) {
            lec_ringUsed = 0;
        }
        else {
            lec_ringUsed--;
        }
        return lec_ring[i];
    }
    return NULL;
}
