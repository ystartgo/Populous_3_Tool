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

static uintptr_t g_ContinueAddr = 0;
static uintptr_t g_ExitAddr = 0;

// Hook function at 0x00428723 (Populous Follower Move Command in D3DPopTB.exe)
__attribute__((naked)) void Hook_MoveCheck() {
    __asm__ (
        // Test if this move command was triggered by Physical Right Click
        "movl %0, %%ecx\n\t"
        "testl %%ecx, %%ecx\n\t"
        "jz 1f\n\t"
        
        // Physical Right Click:
        // Reset flag to 0
        "movl $0, %0\n\t"
        // Execute original instructions:
        // xor edx, edx
        // test eax, eax
        // jz exit
        "xorl %%edx, %%edx\n\t"
        "testl %%eax, %%eax\n\t"
        "jz 1f\n\t"
        // Jump to continue address (0x00428729) to issue Command 7 (MOVE)
        "pushl %1\n\t"
        "ret\n\t"
        
        // Physical Left Click or eax==0:
        // Jump directly to exit address (0x004287a8) to suppress move command!
        "1:\n\t"
        "pushl %2\n\t"
        "ret\n\t"
        :
        : "m"(g_IsPhysicalRightClick), "m"(g_ContinueAddr), "m"(g_ExitAddr)
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
    
    // Exact signature in D3DPopTB.exe for follower move command check:
    // xor edx, edx; test eax, eax; jz +0x7f; xor edi, edi; cmp byte ptr [0x596c88], dl
    static const BYTE sig[] = { 0x33, 0xd2, 0x85, 0xc0, 0x74, 0x7f, 0x33, 0xff, 0x38, 0x15, 0x88, 0x6c, 0x59, 0x00 };
    BYTE* target = NULL;
    
    // Fast path: check known VA 0x00428723
    if ((uintptr_t)hMod == 0x00400000 && !IsBadReadPtr((void*)0x00428723, sizeof(sig))) {
        if (memcmp((void*)0x00428723, sig, sizeof(sig)) == 0) {
            target = (BYTE*)0x00428723;
        }
    }
    
    // Fallback: scan image
    if (!target) {
        for (DWORD i = 0; i < size - sizeof(sig); i++) {
            if (base[i] == 0x33 && base[i+1] == 0xd2 && base[i+2] == 0x85 && base[i+3] == 0xc0) {
                if (memcmp(base + i, sig, sizeof(sig)) == 0) {
                    target = base + i;
                    break;
                }
            }
        }
    }
    
    if (!target) {
        Log("Engine signature for MoveCheck not found in process image\n");
        return;
    }
    
    g_ContinueAddr = (uintptr_t)(target + 6);
    signed char relExit = (signed char)target[5];
    g_ExitAddr = (uintptr_t)(target + 6 + relExit);
    
    Log("Found MoveCheck at %p: continue=%p, exit=%p\n", target, (void*)g_ContinueAddr, (void*)g_ExitAddr);
    
    DWORD oldProtect;
    if (VirtualProtect(target, 16, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        DWORD rel = (DWORD)(uintptr_t)Hook_MoveCheck - (DWORD)(uintptr_t)target - 5;
        target[0] = 0xE9; // JMP rel32
        *(DWORD*)(target + 1) = rel;
        target[5] = 0x90; // NOP
        VirtualProtect(target, 16, oldProtect, &oldProtect);
        Log("Installed Left-Click Move Suppression Hook at %p successfully!\n", target);
    } else {
        Log("Failed to VirtualProtect target at %p\n", target);
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
                        // Mark physical right click active for engine move handler
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
            if (ms->rgbButtons[1] & 0x80) { // Right button down
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
