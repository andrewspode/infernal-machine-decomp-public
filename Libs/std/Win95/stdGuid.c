/*
 * DirectX interface and object GUIDs, defined once for the whole program.
 *
 * With INITGUID defined, every DEFINE_GUID in the DirectX headers becomes a
 * definition, so this object holds the GUIDs of dinput.h, ddraw.h, dplay.h,
 * dplobby.h, dsound.h and d3d.h, each header's in its own order, in place of
 * dxguid.lib. Retail's .rdata block 0x00508F80-0x00509710 is exactly this
 * object: 121 GUIDs in that order, most of them never referenced by code.
 */
#define INITGUID
#include <windows.h>
#include <dinput.h>
#include <ddraw.h>
#include <dplobby.h>

/* Four GUIDs from no DirectX or Visual C++ header; retail places them after
   the DirectPlay lobby GUIDs and nothing references them. */
DEFINE_GUID(stdGuid_unknown_5094C0, 0xBDD4B95C, 0xD35C, 0x11D0, 0xB6, 0x25, 0x00, 0xC0, 0x4F, 0xC3, 0x3E, 0xA1);
DEFINE_GUID(stdGuid_unknown_5094D0, 0x33B64CA7, 0xD8EB, 0x11D0, 0xB6, 0x2B, 0x00, 0xC0, 0x4F, 0xC3, 0x3E, 0xA1);
DEFINE_GUID(stdGuid_unknown_5094E0, 0xBDD4B95F, 0xD35C, 0x11D0, 0xB6, 0x25, 0x00, 0xC0, 0x4F, 0xC3, 0x3E, 0xA1);
DEFINE_GUID(stdGuid_unknown_5094F0, 0x058EBD64, 0x1373, 0x11D1, 0x96, 0x8F, 0x00, 0xC0, 0x4F, 0xC2, 0xDB, 0x04);

#include <dsound.h>
#include <d3d.h>
