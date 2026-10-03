#define DIRECTINPUT_VERSION 0x0500
#define CINTERFACE
#include <windows.h>
#include <dinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static const GUID MY_GUID_SysMouse = { 0x6F1D2B60, 0xD5A0, 0x11CF, { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

static HMODULE g_hRealDInput = NULL;
static IDirectInputDeviceA* g_pMouseDevice = NULL;

static int g_EnableRightClickMove = 1;
static int g_ModernControls = 1;
static int g_ShiftReverts = 1;
static int g_DebugLog = 0;

// Flag to track whether the active ground action was initiated by Physical Right Click
static volatile int g_IsPhysicalRightClick = 0;

// 1. Single-unit / Shaman Move Hook (VA 0x00427b20)
static uintptr_t g_SingleMoveTarget = 0x00427b20;
static uintptr_t g_SingleMoveContinue = 0x00427b27;

__attribute__((naked)) void Hook_SingleMove() {
    __asm__ (
        // Test if this is a physical right click
        "movl %0, %%ecx\n\t"
        "testl %%ecx, %%ecx\n\t"
        "jnz 2f\n\t"
        
        // Physical Left Click: reject move! Return 0 cleanly
        "1:\n\t"
        "xorl %%eax, %%eax\n\t"
        "ret\n\t"
        
        // Physical Right Click: allow move
        "2:\n\t"
        "movl $0, %0\n\t"
        // Execute original stolen 7 bytes:
        // push esi
        // push 0
        // mov esi, dword ptr [esp + 0xc]
        "pushl %%esi\n\t"
        "pushl $0\n\t"
        "movl 0xc(%%esp), %%esi\n\t"
        // Jump to continue address
        "pushl %1\n\t"
        "ret\n\t"
        :
        : "m"(g_IsPhysicalRightClick), "m"(g_SingleMoveContinue)
    );
}

// 2. Multi-unit Group Move Hook (VA 0x004286e0)
static uintptr_t g_MultiMoveTarget = 0x004286e0;
static uintptr_t g_MultiMoveContinue = 0x004286e7;

__attribute__((naked)) void Hook_MultiMove() {
    __asm__ (
        // Test if this is a physical right click
        "movl %0, %%ecx\n\t"
        "testl %%ecx, %%ecx\n\t"
        "jnz 2f\n\t"
        
        // Physical Left Click: reject move! Return immediately without executing move loop
        "1:\n\t"
        "ret\n\t"
        
        // Physical Right Click: allow move
        "2:\n\t"
        "movl $0, %0\n\t"
        // Execute original stolen 7 bytes:
        // mov eax, dword ptr [esp + 4]
        // sub esp, 8
        "movl 0x4(%%esp), %%eax\n\t"
        "subl $8, %%esp\n\t"
        // Jump to continue address
        "pushl %1\n\t"
        "ret\n\t"
        :
        : "m"(g_IsPhysicalRightClick), "m"(g_MultiMoveContinue)
    );
}

typedef HRESULT (WINAPI *DirectInputCreateA_t)(HINSTANCE, DWORD, LPDIRECTINPUTA*, LPUNKNOWN);
typedef HRESULT (WINAPI *DirectInputCreateW_t)(HINSTANCE, DWORD, LPDIRECTINPUTW*, LPUNKNOWN);
typedef HRESULT (WINAPI *DirectInputCreateEx_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
typedef HRESULT (STDAPICALLTYPE *DllCanUnloadNow_t)(void);
typedef HRESULT (STDAPICALLTYPE *DllGetClassObject_t)(REFCLSID, REFIID, LPVOID*);
typedef HRESULT (STDAPICALLTYPE *DllRegisterServer_t)(void);
typedef HRESULT (STDAPICALLTYPE *DllUnregisterServer_t)(void);

static DirectInputCreateA_t pfnDirectInputCreateA = NULL;
static DirectInputCreateW_t pfnDirectInputCreateW = NULL;
static DirectInputCreateEx_t pfnDirectInputCreateEx = NULL;
static DllCanUnloadNow_t pfnDllCanUnloadNow = NULL;
static DllGetClassObject_t pfnDllGetClassObject = NULL;
static DllRegisterServer_t pfnDllRegisterServer = NULL;
static DllUnregisterServer_t pfnDllUnregisterServer = NULL;

typedef HRESULT (STDMETHODCALLTYPE *CreateDevice_t)(IDirectInputA*, REFGUID, LPDIRECTINPUTDEVICEA*, LPUNKNOWN);
typedef HRESULT (STDMETHODCALLTYPE *GetDeviceData_t)(IDirectInputDeviceA*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
typedef HRESULT (STDMETHODCALLTYPE *GetDeviceState_t)(IDirectInputDeviceA*, DWORD, LPVOID);

static CreateDevice_t real_CreateDevice = NULL;
static GetDeviceData_t real_GetDeviceData = NULL;
static GetDeviceState_t real_GetDeviceState = NULL;

static void Log(const char* fmt, ...) {
    if (!g_DebugLog) return;
    FILE* f = fopen("populous_mouse.log", "a");
    if (!f) return;
    va_list va;
    va_start(va, fmt);
    vfprintf(f, fmt, va);
    va_end(va);
    fclose(f);
}

static void InstallEnginePatch() {
    HMODULE hMod = GetModuleHandleA(NULL);
    if (!hMod) return;
    
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)hMod;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE*)hMod + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return;
    
    BYTE* base = (BYTE*)hMod;
    DWORD size = nt->OptionalHeader.SizeOfImage;
    
    // 1. Single-Unit / Shaman Move Hook at 0x00427b20
    // Signature: 56 6a 00 8b 74 24 0c 6a 07 56
    static const BYTE sigSingle[] = { 0x56, 0x6a, 0x00, 0x8b, 0x74, 0x24, 0x0c, 0x6a, 0x07, 0x56 };
    BYTE* targetSingle = NULL;
    if ((uintptr_t)hMod == 0x00400000 && !IsBadReadPtr((void*)0x00427b20, sizeof(sigSingle))) {
        if (memcmp((void*)0x00427b20, sigSingle, sizeof(sigSingle)) == 0) {
            targetSingle = (BYTE*)0x00427b20;
        }
    }
    if (!targetSingle) {
        for (DWORD i = 0; i < size - sizeof(sigSingle); i++) {
            if (base[i] == 0x56 && base[i+1] == 0x6a && base[i+2] == 0x00 && base[i+3] == 0x8b) {
                if (memcmp(base + i, sigSingle, sizeof(sigSingle)) == 0) {
                    targetSingle = base + i;
                    break;
                }
            }
        }
    }
    if (targetSingle) {
        g_SingleMoveContinue = (uintptr_t)(targetSingle + 7);
        DWORD oldProtect;
        if (VirtualProtect(targetSingle, 16, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            DWORD rel = (DWORD)(uintptr_t)Hook_SingleMove - (DWORD)(uintptr_t)targetSingle - 5;
            targetSingle[0] = 0xE9; // JMP rel32
            *(DWORD*)(targetSingle + 1) = rel;
            targetSingle[5] = 0x90; // NOP
            targetSingle[6] = 0x90; // NOP
            VirtualProtect(targetSingle, 16, oldProtect, &oldProtect);
            Log("Installed Hook_SingleMove at %p successfully!\n", targetSingle);
        }
    } else {
        Log("Failed to locate SingleMove function in process\n");
    }
    
    // 2. Multi-Unit Group Move Hook at 0x004286e0
    // Signature: 8b 44 24 04 83 ec 08 53 56 57 55 50
    static const BYTE sigMulti[] = { 0x8b, 0x44, 0x24, 0x04, 0x83, 0xec, 0x08, 0x53, 0x56, 0x57, 0x55, 0x50 };
    BYTE* targetMulti = NULL;
    if ((uintptr_t)hMod == 0x00400000 && !IsBadReadPtr((void*)0x004286e0, sizeof(sigMulti))) {
        if (memcmp((void*)0x004286e0, sigMulti, sizeof(sigMulti)) == 0) {
            targetMulti = (BYTE*)0x004286e0;
        }
    }
    if (!targetMulti) {
        for (DWORD i = 0; i < size - sizeof(sigMulti); i++) {
            if (base[i] == 0x8b && base[i+1] == 0x44 && base[i+2] == 0x24 && base[i+3] == 0x04) {
                if (memcmp(base + i, sigMulti, sizeof(sigMulti)) == 0) {
                    targetMulti = base + i;
                    break;
                }
            }
        }
    }
    if (targetMulti) {
        g_MultiMoveContinue = (uintptr_t)(targetMulti + 7);
        DWORD oldProtect;
        if (VirtualProtect(targetMulti, 16, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            DWORD rel = (DWORD)(uintptr_t)Hook_MultiMove - (DWORD)(uintptr_t)targetMulti - 5;
            targetMulti[0] = 0xE9; // JMP rel32
            *(DWORD*)(targetMulti + 1) = rel;
            targetMulti[5] = 0x90; // NOP
            targetMulti[6] = 0x90; // NOP
            VirtualProtect(targetMulti, 16, oldProtect, &oldProtect);
            Log("Installed Hook_MultiMove at %p successfully!\n", targetMulti);
        }
    } else {
        Log("Failed to locate MultiMove function in process\n");
    }
}

static void LoadConfig() {
    char iniPath[MAX_PATH];
    GetModuleFileNameA(NULL, iniPath, MAX_PATH);
    char* lastSlash = strrchr(iniPath, '\\');
    if (lastSlash) {
        strcpy(lastSlash + 1, "populous_mouse.ini");
    } else {
        strcpy(iniPath, "populous_mouse.ini");
    }
    g_EnableRightClickMove = GetPrivateProfileIntA("Mouse", "EnableRightClickMove", 1, iniPath);
    g_ModernControls = GetPrivateProfileIntA("Mouse", "ModernControls", 1, iniPath);
    g_ShiftReverts = GetPrivateProfileIntA("Mouse", "ShiftRevertsToRightClick", 1, iniPath);
    g_DebugLog = GetPrivateProfileIntA("Mouse", "DebugLog", 0, iniPath);
}

static void LoadRealDInput() {
    if (g_hRealDInput) return;
    LoadConfig();
    char sysPath[MAX_PATH];
    GetSystemDirectoryA(sysPath, MAX_PATH);
    strcat(sysPath, "\\dinput.dll");
    g_hRealDInput = LoadLibraryA(sysPath);
    if (!g_hRealDInput) {
        Log("Failed to load system dinput.dll from %s\n", sysPath);
        return;
    }
    pfnDirectInputCreateA = (DirectInputCreateA_t)GetProcAddress(g_hRealDInput, "DirectInputCreateA");
    pfnDirectInputCreateW = (DirectInputCreateW_t)GetProcAddress(g_hRealDInput, "DirectInputCreateW");
    pfnDirectInputCreateEx = (DirectInputCreateEx_t)GetProcAddress(g_hRealDInput, "DirectInputCreateEx");
    pfnDllCanUnloadNow = (DllCanUnloadNow_t)GetProcAddress(g_hRealDInput, "DllCanUnloadNow");
    pfnDllGetClassObject = (DllGetClassObject_t)GetProcAddress(g_hRealDInput, "DllGetClassObject");
    pfnDllRegisterServer = (DllRegisterServer_t)GetProcAddress(g_hRealDInput, "DllRegisterServer");
    pfnDllUnregisterServer = (DllUnregisterServer_t)GetProcAddress(g_hRealDInput, "DllUnregisterServer");
    Log("Loaded real dinput.dll successfully. EnableRightClickMove=%d, ModernControls=%d\n",
        g_EnableRightClickMove, g_ModernControls);
    
    if (g_EnableRightClickMove && g_ModernControls) {
        InstallEnginePatch();
    }
}

static HRESULT STDMETHODCALLTYPE Hooked_GetDeviceData(IDirectInputDeviceA* pThis, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags) {
    HRESULT hr = real_GetDeviceData(pThis, cbObjectData, rgdod, pdwInOut, dwFlags);
    if (SUCCEEDED(hr) && pThis == g_pMouseDevice && rgdod != NULL && pdwInOut != NULL && *pdwInOut > 0) {
        if (g_EnableRightClickMove) {
            DWORD count = *pdwInOut;
            for (DWORD i = 0; i < count; i++) {
                LPDIDEVICEOBJECTDATA item = (LPDIDEVICEOBJECTDATA)((BYTE*)rgdod + i * cbObjectData);
                
                if (item->dwOfs == DIMOFS_BUTTON1) { // Physical Right Click
                    BOOL shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                    if (!shift || !g_ShiftReverts) {
                        // Mark physical right click active for engine move handlers
                        if (item->dwData & 0x80) {
                            g_IsPhysicalRightClick = 1;
                        }
                        // Remap Right Click to Left Click (DIMOFS_BUTTON0) for RTS movement
                        item->dwOfs = DIMOFS_BUTTON0;
                        Log("Remapped Right Click -> Left Click (dwData=0x%lx)\n", item->dwData);
                    } else {
                        Log("Passed through Right Click (Shift held)\n");
                    }
                }
                else if (item->dwOfs == DIMOFS_BUTTON0) { // Physical Left Click
                    // Physical Left Click down clears right click flag so ground clicks won't move units
                    if (item->dwData & 0x80) {
                        g_IsPhysicalRightClick = 0;
                    }
                    Log("Left Click -> Select / Box-select (dwData=0x%lx)\n", item->dwData);
                }
            }
        }
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE Hooked_GetDeviceState(IDirectInputDeviceA* pThis, DWORD cbData, LPVOID lpvData) {
    HRESULT hr = real_GetDeviceState(pThis, cbData, lpvData);
    if (SUCCEEDED(hr) && pThis == g_pMouseDevice && lpvData != NULL && cbData >= sizeof(DIMOUSESTATE)) {
        if (g_EnableRightClickMove) {
            DIMOUSESTATE* ms = (DIMOUSESTATE*)lpvData;
            if (ms->rgbButtons[1] & 0x80) { // Physical right button down
                BOOL shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                if (!shift || !g_ShiftReverts) {
                    g_IsPhysicalRightClick = 1;
                    ms->rgbButtons[0] = ms->rgbButtons[1];
                    ms->rgbButtons[1] = 0;
                }
            } else if (ms->rgbButtons[0] & 0x80) {
                // Physical left button down
                g_IsPhysicalRightClick = 0;
            }
        }
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE Hooked_CreateDevice(IDirectInputA* pThis, REFGUID rguid, LPDIRECTINPUTDEVICEA* lplpDirectInputDevice, LPUNKNOWN pUnkOuter) {
    HRESULT hr = real_CreateDevice(pThis, rguid, lplpDirectInputDevice, pUnkOuter);
    if (SUCCEEDED(hr) && lplpDirectInputDevice && *lplpDirectInputDevice) {
        if (IsEqualGUID(rguid, MY_GUID_SysMouse)) {
            g_pMouseDevice = *lplpDirectInputDevice;
            Log("DirectInput mouse device created: %p\n", g_pMouseDevice);
            
            // Hook GetDeviceData and GetDeviceState in device vtable
            DWORD oldProtect;
            if (VirtualProtect(g_pMouseDevice->lpVtbl, sizeof(IDirectInputDeviceAVtbl), PAGE_EXECUTE_READWRITE, &oldProtect)) {
                if (!real_GetDeviceData) {
                    real_GetDeviceData = g_pMouseDevice->lpVtbl->GetDeviceData;
                    g_pMouseDevice->lpVtbl->GetDeviceData = Hooked_GetDeviceData;
                }
                if (!real_GetDeviceState) {
                    real_GetDeviceState = g_pMouseDevice->lpVtbl->GetDeviceState;
                    g_pMouseDevice->lpVtbl->GetDeviceState = Hooked_GetDeviceState;
                }
                VirtualProtect(g_pMouseDevice->lpVtbl, sizeof(IDirectInputDeviceAVtbl), oldProtect, &oldProtect);
                Log("Hooked GetDeviceData (%p -> %p) and GetDeviceState (%p -> %p)\n",
                    real_GetDeviceData, Hooked_GetDeviceData, real_GetDeviceState, Hooked_GetDeviceState);
            } else {
                Log("VirtualProtect on device vtable failed\n");
            }
        }
    }
    return hr;
}

extern "C" {

HRESULT WINAPI Proxy_DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA* lplpDirectInput, LPUNKNOWN punkOuter) {
    LoadRealDInput();
    Log("Proxy_DirectInputCreateA called (version=%lx)\n", dwVersion);
    if (!pfnDirectInputCreateA) return E_FAIL;
    HRESULT hr = pfnDirectInputCreateA(hinst, dwVersion, lplpDirectInput, punkOuter);
    if (SUCCEEDED(hr) && lplpDirectInput && *lplpDirectInput) {
        LPDIRECTINPUTA pDI = *lplpDirectInput;
        DWORD oldProtect;
        if (VirtualProtect(pDI->lpVtbl, sizeof(IDirectInputAVtbl), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            if (!real_CreateDevice) {
                real_CreateDevice = pDI->lpVtbl->CreateDevice;
                pDI->lpVtbl->CreateDevice = Hooked_CreateDevice;
            }
            VirtualProtect(pDI->lpVtbl, sizeof(IDirectInputAVtbl), oldProtect, &oldProtect);
            Log("Hooked CreateDevice (%p -> %p)\n", real_CreateDevice, Hooked_CreateDevice);
        } else {
            Log("VirtualProtect on IDirectInput vtable failed\n");
        }
    }
    return hr;
}

HRESULT WINAPI Proxy_DirectInputCreateW(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTW* lplpDirectInput, LPUNKNOWN punkOuter) {
    LoadRealDInput();
    if (!pfnDirectInputCreateW) return E_FAIL;
    return pfnDirectInputCreateW(hinst, dwVersion, lplpDirectInput, punkOuter);
}

HRESULT WINAPI Proxy_DirectInputCreateEx(HINSTANCE hinst, DWORD dwVersion, REFIID riid, LPVOID* lplpDirectInput, LPUNKNOWN punkOuter) {
    LoadRealDInput();
    if (!pfnDirectInputCreateEx) return E_FAIL;
    return pfnDirectInputCreateEx(hinst, dwVersion, riid, lplpDirectInput, punkOuter);
}

HRESULT STDAPICALLTYPE Proxy_DllCanUnloadNow(void) {
    LoadRealDInput();
    if (!pfnDllCanUnloadNow) return S_OK;
    return pfnDllCanUnloadNow();
}

HRESULT STDAPICALLTYPE Proxy_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    LoadRealDInput();
    if (!pfnDllGetClassObject) return E_FAIL;
    return pfnDllGetClassObject(rclsid, riid, ppv);
}

HRESULT STDAPICALLTYPE Proxy_DllRegisterServer(void) {
    LoadRealDInput();
    if (!pfnDllRegisterServer) return E_FAIL;
    return pfnDllRegisterServer();
}

HRESULT STDAPICALLTYPE Proxy_DllUnregisterServer(void) {
    LoadRealDInput();
    if (!pfnDllUnregisterServer) return E_FAIL;
    return pfnDllUnregisterServer();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}

}
