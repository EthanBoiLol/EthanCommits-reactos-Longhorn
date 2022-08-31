
#include "DWrite.h"
#include <psdk/winnls.h>
#include <debug.h>

BOOL
WINAPI
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    /* For now, there isn't much to do */
    if (dwReason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hDll);
    return TRUE;
}

HRESULT
WINAPI
DWriteCreateFactory(DWRITE_FACTORY_TYPE type, REFIID riid, IUnknown **ret)
{
    UNIMPLEMENTED;
    return 0;
}