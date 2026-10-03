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
static int g_MiddleClickRotatesCamera = 1;
static int g_DebugLog = 1;

// Tracks physical right click status and timestamp
static volatile int g_IsPhysicalRightClick = 0;
static volatile DWORD g_LastRightClickTick = 0;

static void Log(const char* fmt, ...) {
    if (!g_DebugLog) return;
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* s = strrchr(path, '\\');
    if (s) strcpy(s + 1, "populous_mouse.log");
    else strcpy(path, "populous_mouse.log");
    
    FILE* f = fopen(path, "a");
    if (!f) return;
    va_list va;
    va_start(va, fmt);
    vfprintf(f, fmt, va);
    va_end(va);
    fclose(f);
}

static void HideSystemCursor() {
    // Keep internal cursor display counter negative so Windows desktop cursor is never drawn
    while (ShowCursor(FALSE) >= 0);
}

// Check whether a command queued through 0x004d71d0 is allowed
extern "C" int __attribute__((cdecl)) CheckCommandAllowed(int unitIdx, int playerIdx, int cmdId, int target) {
    // If not Move command (cmdId != 7), always allow (e.g. spells, attack, build, etc.)
    if (cmdId != 7) {
        return 1;
    }
    
    // If modern controls disabled, allow original behavior
    if (!g_EnableRightClickMove || !g_ModernControls) {
        return 1;
    }
    
    // If command is for AI players (playerIdx != 0 in single-player), allow unconditionally
    if (playerIdx != 0) {
        return 1;
    }
    
    // Command 7 (MOVE) for Player 0 (local human player):
    DWORD now = GetTickCount();
    int isRightClick = g_IsPhysicalRightClick || 
                       ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) ||
                       ((now - g_LastRightClickTick) < 500);
                       
    if (isRightClick) {
        Log("[CMD-ALLOW] Move command 7 allowed (unit=%d, target=0x%x)\n", unitIdx, target);
        return 1;
    } else {
        Log("[CMD-BLOCK] Blocked Left-Click Move command 7 (unit=%d, target=0x%x)\n", unitIdx, target);
        return 0; // REJECT: prevents left click on ground from moving units!
    }
}

// Hook on QueuePlayerCommand at VA 0x004d71d0
static uintptr_t g_QueueCommandContinue = 0x004d71d5;

__attribute__((naked)) void Hook_QueueCommand() {
    __asm__ (
        // Stack at function entry:
        // [esp + 0x00]: return address
        // [esp + 0x04]: unitIdx
        // [esp + 0x08]: playerIdx
        // [esp + 0x0C]: cmdId
        // [esp + 0x10]: target
        
        // Push arguments for CheckCommandAllowed(unitIdx, playerIdx, cmdId, target)
        "pushl 0x10(%%esp)\n\t" // target (at esp + 0x10)
        "pushl 0x10(%%esp)\n\t" // cmdId (was at 0x0C, now at 0x10)
        "pushl 0x10(%%esp)\n\t" // playerIdx (was at 0x08, now at 0x10)
        "pushl 0x10(%%esp)\n\t" // unitIdx (was at 0x04, now at 0x10)
        
        "call *%1\n\t"          // call CheckCommandAllowed
        "addl $16, %%esp\n\t"   // clean up arguments
        
        "testl %%eax, %%eax\n\t"
        "jnz 1f\n\t"
        
        // Command blocked: return immediately without queueing
        "ret\n\t"
        
        // Command allowed: execute original stolen 5 bytes:
        // 0x4d71d0: mov edx, dword ptr [esp + 8]
        // 0x4d71d4: push esi
        "1:\n\t"
        "movl 0x8(%%esp), %%edx\n\t"
        "pushl %%esi\n\t"
        
        // Jump to continue address 0x004d71d5
        "pushl %0\n\t"
        "ret\n\t"
        :
        : "m"(g_QueueCommandContinue), "r"(CheckCommandAllowed)
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

static void InstallEnginePatch() {
    HMODULE hMod = GetModuleHandleA(NULL);
    if (!hMod) return;
    
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)hMod;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE*)hMod + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return;
    
    BYTE* base = (BYTE*)hMod;
    DWORD size = nt->OptionalHeader.SizeOfImage;
    
    // Signature of 0x004d71d0:
    // 8b 54 24 08 56 8b 4c 24 08 8d 04 d2 8d 04 c0 03
    static const BYTE sigQueueCmd[] = { 0x8b, 0x54, 0x24, 0x08, 0x56, 0x8b, 0x4c, 0x24, 0x08, 0x8d, 0x04, 0xd2, 0x8d, 0x04, 0xc0, 0x03 };
    BYTE* targetQueueCmd = NULL;
    if ((uintptr_t)hMod == 0x00400000 && !IsBadReadPtr((void*)0x004d71d0, sizeof(sigQueueCmd))) {
        if (memcmp((void*)0x004d71d0, sigQueueCmd, sizeof(sigQueueCmd)) == 0) {
            targetQueueCmd = (BYTE*)0x004d71d0;
        }
    }
    if (!targetQueueCmd) {
        for (DWORD i = 0; i < size - sizeof(sigQueueCmd); i++) {
            if (base[i] == 0x8b && base[i+1] == 0x54 && base[i+2] == 0x24 && base[i+3] == 0x08 && base[i+4] == 0x56) {
                if (memcmp(base + i, sigQueueCmd, sizeof(sigQueueCmd)) == 0) {
                    targetQueueCmd = base + i;
                    break;
                }
            }
        }
    }
    if (targetQueueCmd) {
        g_QueueCommandContinue = (uintptr_t)(targetQueueCmd + 5);
        DWORD oldProtect;
        if (VirtualProtect(targetQueueCmd, 16, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            DWORD rel = (DWORD)(uintptr_t)Hook_QueueCommand - (DWORD)(uintptr_t)targetQueueCmd - 5;
            targetQueueCmd[0] = 0xE9; // JMP rel32
            *(DWORD*)(targetQueueCmd + 1) = rel;
            VirtualProtect(targetQueueCmd, 16, oldProtect, &oldProtect);
            Log("Installed Hook_QueueCommand at %p successfully!\n", targetQueueCmd);
        }
    } else {
        Log("Failed to locate QueuePlayerCommand at 0x004d71d0 in process\n");
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
    g_MiddleClickRotatesCamera = GetPrivateProfileIntA("Mouse", "MiddleClickRotatesCamera", 1, iniPath);
    g_DebugLog = GetPrivateProfileIntA("Mouse", "DebugLog", 1, iniPath);
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
    HideSystemCursor();
    HRESULT hr = real_GetDeviceData(pThis, cbObjectData, rgdod, pdwInOut, dwFlags);
    if (SUCCEEDED(hr) && rgdod != NULL && pdwInOut != NULL && *pdwInOut > 0) {
        if (g_EnableRightClickMove) {
            DWORD count = *pdwInOut;
            for (DWORD i = 0; i < count; i++) {
                LPDIDEVICEOBJECTDATA item = (LPDIDEVICEOBJECTDATA)((BYTE*)rgdod + i * cbObjectData);
                
                // Middle click -> original right click (Camera Rotate)
                if (item->dwOfs == DIMOFS_BUTTON2 && g_MiddleClickRotatesCamera) {
                    item->dwOfs = DIMOFS_BUTTON1;
                    Log("[DINPUT] Remapped Middle Click -> Original Right Click (Camera Rotate)\n");
                }
                // Physical Right Click -> Left Click (Orders the action in-game)
                else if (item->dwOfs == DIMOFS_BUTTON1) {
                    BOOL shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                    if (!shift || !g_ShiftReverts) {
                        if (item->dwData & 0x80) { // Pressed down
                            g_IsPhysicalRightClick = 1;
                            g_LastRightClickTick = GetTickCount();
                        } else { // Released
                            g_IsPhysicalRightClick = 0;
                        }
                        item->dwOfs = DIMOFS_BUTTON0;
                        Log("[DINPUT-DATA] Remapped Right Click -> Left Click (dwData=0x%lx)\n", item->dwData);
                    } else {
                        Log("[DINPUT-DATA] Passed through Right Click (Shift held)\n");
                    }
                }
                // Physical Left Click
                else if (item->dwOfs == DIMOFS_BUTTON0) {
                    if (item->dwData & 0x80) { // Pressed down
                        g_IsPhysicalRightClick = 0;
                        g_LastRightClickTick = 0;
                    }
                }
            }
        }
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE Hooked_GetDeviceState(IDirectInputDeviceA* pThis, DWORD cbData, LPVOID lpvData) {
    HideSystemCursor();
    HRESULT hr = real_GetDeviceState(pThis, cbData, lpvData);
    if (SUCCEEDED(hr) && lpvData != NULL && cbData >= sizeof(DIMOUSESTATE)) {
        if (g_EnableRightClickMove) {
            DIMOUSESTATE* ms = (DIMOUSESTATE*)lpvData;
            // Middle button -> right button
            if ((ms->rgbButtons[2] & 0x80) && g_MiddleClickRotatesCamera) {
                ms->rgbButtons[1] = ms->rgbButtons[2];
                ms->rgbButtons[2] = 0;
            }
            // Physical right button
            if (ms->rgbButtons[1] & 0x80) {
                BOOL shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                if (!shift || !g_ShiftReverts) {
                    g_IsPhysicalRightClick = 1;
                    g_LastRightClickTick = GetTickCount();
                    ms->rgbButtons[0] = ms->rgbButtons[1];
                    ms->rgbButtons[1] = 0;
                }
            } else if (ms->rgbButtons[0] & 0x80) {
                g_IsPhysicalRightClick = 0;
                g_LastRightClickTick = 0;
            }
        }
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE Hooked_CreateDevice(IDirectInputA* pThis, REFGUID rguid, LPDIRECTINPUTDEVICEA* lplpDirectInputDevice, LPUNKNOWN pUnkOuter) {
    HRESULT hr = real_CreateDevice(pThis, rguid, lplpDirectInputDevice, pUnkOuter);
    if (SUCCEEDED(hr) && lplpDirectInputDevice && *lplpDirectInputDevice) {
        g_pMouseDevice = *lplpDirectInputDevice;
        Log("DirectInput device created: %p (rguid=%08x)\n", g_pMouseDevice, rguid.Data1);
        HideSystemCursor();
        
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
        HideSystemCursor();
        LoadRealDInput();
    }
    return TRUE;
}

}
