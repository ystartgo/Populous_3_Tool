// Populous: The Beginning - Modern RTS mouse controls (dinput.dll proxy)
//
// How it works:
//   The engine turns mouse buttons into "actions" through a context-sensitive
//   key binding table (lookup function at VA 0x004172A0, __thiscall,
//   args: keycode, modifiers, type[1=press,4=release]).
//   Mouse key codes: 0xF0 = left, 0xF1 = right, 0xF2 = middle.
//
//   When followers are selected the input mode byte [0x0067362F] == 0x0D:
//     left  release -> action 106/109 (packet 0x6E: move selected to target)
//     right release -> action 108     (packet 0x6F: deselect)
//
//   Modern RTS mode swaps these two ONLY in that mode:
//     right release -> move,  left release on ground -> deselect.
//   Clicking a follower with left (select) is handled before the binding
//   lookup, so selection keeps working. Spells / building placement / UI
//   keep their original bindings. Raw DirectInput buttons are NOT remapped.

#define DIRECTINPUT_VERSION 0x0500
#define CINTERFACE
#include <windows.h>
#include <dinput.h>
#include <stdio.h>
#include <stdint.h>

static HMODULE g_hRealDInput = NULL;
static int g_EnableRightClickMove = 1;
static int g_ModernControls = 1;
static int g_DebugLog = 1;

static const uintptr_t VA_LOOKUP    = 0x004172A0;
static const uintptr_t VA_INPUTMODE = 0x0067362F;
static const BYTE MODE_SELECTED     = 0x0D;
static const int KEY_LMB = 0xF0, KEY_RMB = 0xF1;
static const int TYPE_RELEASE = 4;

static void Log(const char* fmt, ...) {
    if (!g_DebugLog) return;
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* s = strrchr(path, '\\');
    strcpy(s ? s + 1 : path, "populous_mouse.log");
    FILE* f = fopen(path, "a");
    if (!f) return;
    va_list va; va_start(va, fmt); vfprintf(f, fmt, va); va_end(va);
    fclose(f);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Binding lookup hook
// ---------------------------------------------------------------------------
extern "C" int __attribute__((thiscall)) Tramp_Lookup(void* table, int key, int mods, int type);

__asm__(
".text\n\t"
".globl _Tramp_Lookup\n\t"
"_Tramp_Lookup:\n\t"
"    pushl %ebx\n\t"
"    xorl %eax, %eax\n\t"
"    movb 8(%esp), %al\n\t"
"    pushl $0x004172A7\n\t"
"    ret\n\t"
);

static int g_ShiftReverts = 1;

extern "C" int __attribute__((cdecl)) MyLookup(void* table, int key, int mods, int type) {
    int k = key & 0xFF;
    int t = type & 0xFF;
    BYTE mode = *(volatile BYTE*)VA_INPUTMODE;

    int origAction = Tramp_Lookup(table, key, mods, type);

    if (g_EnableRightClickMove && g_ModernControls && t == TYPE_RELEASE) {
        // LMB release: If it was going to MOVE followers (Action 126), turn it into DESELECT (Action 131)
        // Spells (Action 106: Cast Spell) are left 100% untouched so Left-Click casts spells normally!
        if (k == KEY_LMB && origAction == 126) {
            Log("[MODERN] LMB release move(126) -> DESELECT(131) mode=0x%02x\n", mode);
            return 131;
        }

        // RMB release: If it was going to DESELECT followers (Action 131), turn it into MOVE (Action 126)
        // Spells (Action 108: Cancel Spell) are left 100% untouched!
        if (k == KEY_RMB && origAction == 131) {
            if (g_ShiftReverts && (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
                Log("[SHIFT-RMB] Keep orig action=131 mode=0x%02x\n", mode);
                return 131;
            }
            Log("[MODERN] RMB release deselect(131) -> MOVE(126) mode=0x%02x\n", mode);
            return 126;
        }
    }

    if (k == KEY_LMB || k == KEY_RMB) {
        Log("[PASS] mode=0x%02x key=0x%02x type=%d action=%d\n", mode, k, t, origAction);
    }
    return origAction;
}

// Entry from 0x004172A0: ecx = table, [esp+4]=key, [esp+8]=mods, [esp+0xC]=type
extern "C" void Hook_Lookup();

__asm__(
".text\n\t"
".globl _Hook_Lookup\n\t"
"_Hook_Lookup:\n\t"
"    pushl %ebp\n\t"
"    movl %esp, %ebp\n\t"
"    pushl 16(%ebp)\n\t"  // type
"    pushl 12(%ebp)\n\t"  // mods
"    pushl 8(%ebp)\n\t"   // key
"    pushl %ecx\n\t"      // table
"    call _MyLookup\n\t"
"    addl $16, %esp\n\t"
"    popl %ebp\n\t"
"    ret $12\n\t"
);

// ---------------------------------------------------------------------------
// Focus & Alt-Tab recovery hook (VA 0x005015C0)
// ---------------------------------------------------------------------------
static const uintptr_t VA_ISAPPACTIVE = 0x005015C0;
static const uintptr_t VA_ACTFLAG     = 0x0059D828;
static const uintptr_t VA_INPUTMGR    = 0x00966430;
static const uintptr_t VA_NOTIFYINPUT = 0x0051CC40;

typedef void (__attribute__((thiscall)) *NotifyInput_t)(void* mgr, int event, int state);

extern "C" int __attribute__((cdecl)) MyIsAppActive() {
    HWND hFg = GetForegroundWindow();
    DWORD pid = 0;
    if (hFg) {
        GetWindowThreadProcessId(hFg, &pid);
    }
    bool isForeground = (pid == GetCurrentProcessId());
    static bool s_wasForeground = true;

    if (isForeground) {
        DWORD curFlag = *(volatile DWORD*)VA_ACTFLAG;
        if (!s_wasForeground || curFlag == 0) {
            *(volatile DWORD*)VA_ACTFLAG = 1;
            NotifyInput_t notify = (NotifyInput_t)VA_NOTIFYINPUT;
            notify((void*)VA_INPUTMGR, 6, 1);
            Log("[FOCUS] Window regained focus (was=%d, flag=%d) -> reacquired input devices!\n", s_wasForeground, curFlag);
            s_wasForeground = true;
        }
        return 1;
    } else {
        s_wasForeground = false;
        return *(volatile DWORD*)VA_ACTFLAG;
    }
}

static void InstallEnginePatch() {
    static const BYTE sig[] = { 0x53, 0x33, 0xC0, 0x8A, 0x44, 0x24, 0x08, 0x56, 0x57, 0x55, 0x8B, 0x34 };
    BYTE* target = (BYTE*)VA_LOOKUP;
    if (IsBadReadPtr(target, sizeof(sig)) || memcmp(target, sig, sizeof(sig)) != 0) {
        Log("Binding lookup signature mismatch at %p - patch NOT installed\n", target);
        return;
    }
    DWORD old;
    if (VirtualProtect(target, 8, PAGE_EXECUTE_READWRITE, &old)) {
        target[0] = 0xE9;
        *(DWORD*)(target + 1) = (DWORD)(uintptr_t)Hook_Lookup - (DWORD)(uintptr_t)target - 5;
        target[5] = 0x90; target[6] = 0x90;
        VirtualProtect(target, 8, old, &old);
        FlushInstructionCache(GetCurrentProcess(), target, 8);
        Log("Installed binding lookup hook at %p\n", target);
    }

    // Install focus / Alt-Tab recovery hook at 0x005015C0
    static const BYTE sigFocus[] = { 0xA1, 0x28, 0xD8, 0x59, 0x00, 0xC3 };
    BYTE* targetFocus = (BYTE*)VA_ISAPPACTIVE;
    if (!IsBadReadPtr(targetFocus, sizeof(sigFocus)) && memcmp(targetFocus, sigFocus, sizeof(sigFocus)) == 0) {
        if (VirtualProtect(targetFocus, 6, PAGE_EXECUTE_READWRITE, &old)) {
            targetFocus[0] = 0xE9;
            *(DWORD*)(targetFocus + 1) = (DWORD)(uintptr_t)MyIsAppActive - (DWORD)(uintptr_t)targetFocus - 5;
            targetFocus[5] = 0x90;
            VirtualProtect(targetFocus, 6, old, &old);
            FlushInstructionCache(GetCurrentProcess(), targetFocus, 6);
            Log("Installed focus recovery hook at %p\n", targetFocus);
        }
    } else {
        Log("Focus recovery signature mismatch at %p\n", targetFocus);
    }
}

// ---------------------------------------------------------------------------
// Config + real dinput forwarding
// ---------------------------------------------------------------------------
static void LoadConfig() {
    char ini[MAX_PATH];
    GetModuleFileNameA(NULL, ini, MAX_PATH);
    char* s = strrchr(ini, '\\');
    strcpy(s ? s + 1 : ini, "populous_mouse.ini");
    g_EnableRightClickMove = GetPrivateProfileIntA("Mouse", "EnableRightClickMove", 1, ini);
    g_ModernControls       = GetPrivateProfileIntA("Mouse", "ModernControls", 1, ini);
    g_ShiftReverts         = GetPrivateProfileIntA("Mouse", "ShiftRevertsToRightClick", 1, ini);
    g_DebugLog             = GetPrivateProfileIntA("Mouse", "DebugLog", 1, ini);
}

typedef HRESULT (WINAPI *DirectInputCreateA_t)(HINSTANCE, DWORD, LPDIRECTINPUTA*, LPUNKNOWN);
typedef HRESULT (WINAPI *DirectInputCreateW_t)(HINSTANCE, DWORD, LPDIRECTINPUTW*, LPUNKNOWN);
typedef HRESULT (WINAPI *DirectInputCreateEx_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
typedef HRESULT (STDAPICALLTYPE *Void_t)(void);
typedef HRESULT (STDAPICALLTYPE *DllGetClassObject_t)(REFCLSID, REFIID, LPVOID*);

static DirectInputCreateA_t pA; static DirectInputCreateW_t pW; static DirectInputCreateEx_t pEx;
static Void_t pCanUnload, pReg, pUnreg; static DllGetClassObject_t pGetClass;

static void LoadRealDInput() {
    if (g_hRealDInput) return;
    char sys[MAX_PATH];
    GetSystemDirectoryA(sys, MAX_PATH);
    strcat(sys, "\\dinput.dll");
    g_hRealDInput = LoadLibraryA(sys);
    if (!g_hRealDInput) return;
    pA = (DirectInputCreateA_t)GetProcAddress(g_hRealDInput, "DirectInputCreateA");
    pW = (DirectInputCreateW_t)GetProcAddress(g_hRealDInput, "DirectInputCreateW");
    pEx = (DirectInputCreateEx_t)GetProcAddress(g_hRealDInput, "DirectInputCreateEx");
    pCanUnload = (Void_t)GetProcAddress(g_hRealDInput, "DllCanUnloadNow");
    pGetClass = (DllGetClassObject_t)GetProcAddress(g_hRealDInput, "DllGetClassObject");
    pReg = (Void_t)GetProcAddress(g_hRealDInput, "DllRegisterServer");
    pUnreg = (Void_t)GetProcAddress(g_hRealDInput, "DllUnregisterServer");
}

extern "C" {
HRESULT WINAPI Proxy_DirectInputCreateA(HINSTANCE h, DWORD v, LPDIRECTINPUTA* o, LPUNKNOWN u) { LoadRealDInput(); return pA ? pA(h, v, o, u) : E_FAIL; }
HRESULT WINAPI Proxy_DirectInputCreateW(HINSTANCE h, DWORD v, LPDIRECTINPUTW* o, LPUNKNOWN u) { LoadRealDInput(); return pW ? pW(h, v, o, u) : E_FAIL; }
HRESULT WINAPI Proxy_DirectInputCreateEx(HINSTANCE h, DWORD v, REFIID r, LPVOID* o, LPUNKNOWN u) { LoadRealDInput(); return pEx ? pEx(h, v, r, o, u) : E_FAIL; }
HRESULT STDAPICALLTYPE Proxy_DllCanUnloadNow(void) { LoadRealDInput(); return pCanUnload ? pCanUnload() : S_OK; }
HRESULT STDAPICALLTYPE Proxy_DllGetClassObject(REFCLSID c, REFIID r, LPVOID* p) { LoadRealDInput(); return pGetClass ? pGetClass(c, r, p) : E_FAIL; }
HRESULT STDAPICALLTYPE Proxy_DllRegisterServer(void) { LoadRealDInput(); return pReg ? pReg() : E_FAIL; }
HRESULT STDAPICALLTYPE Proxy_DllUnregisterServer(void) { LoadRealDInput(); return pUnreg ? pUnreg() : E_FAIL; }

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinst);
        LoadConfig();
        Log("dinput proxy loaded. EnableRightClickMove=%d ModernControls=%d\n", g_EnableRightClickMove, g_ModernControls);
        if (g_EnableRightClickMove && g_ModernControls && (uintptr_t)GetModuleHandleA(NULL) == 0x00400000)
            InstallEnginePatch();
    }
    return TRUE;
}
}
