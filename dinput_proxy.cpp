#define DIRECTINPUT_VERSION 0x0500
#define CINTERFACE
#include <windows.h>
#include <dinput.h>
#include <stdio.h>
#include <stdlib.h>

static const GUID MY_GUID_SysMouse = { 0x6F1D2B60, 0xD5A0, 0x11CF, { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

static HMODULE g_hRealDInput = NULL;
static IDirectInputDeviceA* g_pMouseDevice = NULL;

static int g_EnableRightClickMove = 1;
static int g_LeftClickDeselects = 1;
static int g_ShiftReverts = 1;
static int g_DragThreshold = 6;
static int g_DebugLog = 0;

static BOOL g_LeftPending = FALSE;
static BOOL g_LeftDragging = FALSE;
static int g_DragDx = 0;
static int g_DragDy = 0;
static DIDEVICEOBJECTDATA g_LeftDownEvent;

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
    g_LeftClickDeselects = GetPrivateProfileIntA("Mouse", "LeftClickDeselects", 1, iniPath);
    g_ShiftReverts = GetPrivateProfileIntA("Mouse", "ShiftRevertsToRightClick", 1, iniPath);
    g_DragThreshold = GetPrivateProfileIntA("Mouse", "DragThreshold", 6, iniPath);
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
    Log("Loaded real dinput.dll successfully. EnableRightClickMove=%d, LeftClickDeselects=%d\n", g_EnableRightClickMove, g_LeftClickDeselects);
}

static HRESULT STDMETHODCALLTYPE Hooked_GetDeviceData(IDirectInputDeviceA* pThis, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags) {
    HRESULT hr = real_GetDeviceData(pThis, cbObjectData, rgdod, pdwInOut, dwFlags);
    if (SUCCEEDED(hr) && pThis == g_pMouseDevice && rgdod != NULL && pdwInOut != NULL && *pdwInOut > 0) {
        if (g_EnableRightClickMove) {
            DWORD inCount = *pdwInOut;
            DIDEVICEOBJECTDATA temp[128];
            DWORD outCount = 0;
            
            for (DWORD i = 0; i < inCount && outCount < 120; i++) {
                LPDIDEVICEOBJECTDATA item = (LPDIDEVICEOBJECTDATA)((BYTE*)rgdod + i * cbObjectData);
                
                if (item->dwOfs == DIMOFS_BUTTON1) { // Physical Right Click
                    BOOL shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                    if (!shift || !g_ShiftReverts) {
                        // Remap Right Click to Left Click (DIMOFS_BUTTON0) for RTS movement
                        item->dwOfs = DIMOFS_BUTTON0;
                        temp[outCount++] = *item;
                        Log("Remapped Right Click -> Left Click (dwData=%lu)\n", item->dwData);
                    } else {
                        temp[outCount++] = *item;
                        Log("Passed through Right Click (Shift held)\n");
                    }
                }
                else if (item->dwOfs == DIMOFS_BUTTON0) { // Physical Left Click
                    if (g_LeftClickDeselects) {
                        if (item->dwData & 0x80) { // Left Button DOWN
                            g_LeftPending = TRUE;
                            g_LeftDragging = FALSE;
                            g_DragDx = 0;
                            g_DragDy = 0;
                            g_LeftDownEvent = *item;
                            // Wait for drag or button release before emitting
                        } else { // Left Button UP
                            if (g_LeftDragging) {
                                g_LeftDragging = FALSE;
                                temp[outCount++] = *item; // Emit Button 0 UP to complete drag selection
                                Log("Left Drag Finish -> Button 0 UP\n");
                            } else if (g_LeftPending) {
                                g_LeftPending = FALSE;
                                // Single click without drag: emit Button 1 (Deselect / Cancel)
                                DIDEVICEOBJECTDATA rDown = *item;
                                rDown.dwOfs = DIMOFS_BUTTON1;
                                rDown.dwData = 0x80;
                                temp[outCount++] = rDown;
                                
                                DIDEVICEOBJECTDATA rUp = *item;
                                rUp.dwOfs = DIMOFS_BUTTON1;
                                rUp.dwData = 0x00;
                                temp[outCount++] = rUp;
                                Log("Left Click Ground -> Deselect Units (Button 1)\n");
                            } else {
                                temp[outCount++] = *item;
                            }
                        }
                    } else {
                        temp[outCount++] = *item;
                    }
                }
                else if (item->dwOfs == DIMOFS_X || item->dwOfs == DIMOFS_Y) {
                    temp[outCount++] = *item;
                    if (g_LeftClickDeselects && g_LeftPending) {
                        if (item->dwOfs == DIMOFS_X) g_DragDx += abs((int)item->dwData);
                        if (item->dwOfs == DIMOFS_Y) g_DragDy += abs((int)item->dwData);
                        if (g_DragDx + g_DragDy >= g_DragThreshold) {
                            g_LeftPending = FALSE;
                            g_LeftDragging = TRUE;
                            temp[outCount++] = g_LeftDownEvent; // Emit Button 0 DOWN to start drag-box selection
                            Log("Left Drag Start (threshold met) -> Button 0 DOWN\n");
                        }
                    }
                }
                else {
                    temp[outCount++] = *item;
                }
            }
            
            // Copy transformed events back to rgdod
            memcpy(rgdod, temp, outCount * sizeof(DIDEVICEOBJECTDATA));
            *pdwInOut = outCount;
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
                    ms->rgbButtons[0] = ms->rgbButtons[1];
                    ms->rgbButtons[1] = 0;
                }
            }
            if (g_LeftClickDeselects) {
                if (g_LeftPending) {
                    ms->rgbButtons[0] = 0; // Suppress premature ground move command
                } else if (g_LeftDragging) {
                    ms->rgbButtons[0] = 0x80; // Active box-selection
                }
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
