using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace PopulousMouseHelper
{
    class Program
    {
        private const int WH_MOUSE_LL = 14;
        private const int WM_RBUTTONDOWN = 0x0204;
        private const int WM_RBUTTONUP = 0x0205;
        private const int VK_SHIFT = 0x10;

        private const uint MOUSEEVENTF_LEFTDOWN = 0x0002;
        private const uint MOUSEEVENTF_LEFTUP = 0x0004;

        [StructLayout(LayoutKind.Sequential)]
        private struct POINT
        {
            public int x;
            public int y;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct MSLLHOOKSTRUCT
        {
            public POINT pt;
            public uint mouseData;
            public uint flags;
            public uint time;
            public IntPtr dwExtraInfo;
        }

        private delegate IntPtr LowLevelMouseProc(int nCode, IntPtr wParam, IntPtr lParam);
        private static LowLevelMouseProc _proc = HookCallback;
        private static IntPtr _hookID = IntPtr.Zero;
        private static bool _simulatingLeft = false;

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        private static extern IntPtr SetWindowsHookEx(int idHook, LowLevelMouseProc lpfn, IntPtr hMod, uint dwThreadId);

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool UnhookWindowsHookEx(IntPtr hhk);

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        private static extern IntPtr CallNextHookEx(IntPtr hhk, int nCode, IntPtr wParam, IntPtr lParam);

        [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        private static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("user32.dll")]
        private static extern IntPtr GetForegroundWindow();

        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

        [DllImport("user32.dll")]
        private static extern short GetAsyncKeyState(int vKey);

        [DllImport("user32.dll")]
        private static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, UIntPtr dwExtraInfo);

        private static bool IsPopulousActive()
        {
            try
            {
                IntPtr fg = GetForegroundWindow();
                if (fg == IntPtr.Zero) return false;
                uint pid;
                GetWindowThreadProcessId(fg, out pid);
                if (pid == 0) return false;
                Process p = Process.GetProcessById((int)pid);
                string name = p.ProcessName.ToLower();
                return name.Contains("poptb") || name.Contains("d3dpoptb");
            }
            catch
            {
                return false;
            }
        }

        private static IntPtr HookCallback(int nCode, IntPtr wParam, IntPtr lParam)
        {
            if (nCode >= 0 && !_simulatingLeft)
            {
                int msg = wParam.ToInt32();
                if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP)
                {
                    if (IsPopulousActive())
                    {
                        // If Shift is pressed, pass through original Right Click (for deselect/info)
                        bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                        if (!shiftPressed)
                        {
                            _simulatingLeft = true;
                            if (msg == WM_RBUTTONDOWN)
                            {
                                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, UIntPtr.Zero);
                            }
                            else if (msg == WM_RBUTTONUP)
                            {
                                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, UIntPtr.Zero);
                            }
                            _simulatingLeft = false;
                            // Block original right button message
                            return (IntPtr)1;
                        }
                    }
                }
            }
            return CallNextHookEx(_hookID, nCode, wParam, lParam);
        }

        static void Main(string[] args)
        {
            bool createdNew;
            using (var mutex = new System.Threading.Mutex(true, "PopulousMouseHelper_Mutex", out createdNew))
            {
                if (!createdNew) return;

                using (Process curProcess = Process.GetCurrentProcess())
                using (ProcessModule curModule = curProcess.MainModule)
                {
                    _hookID = SetWindowsHookEx(WH_MOUSE_LL, _proc, GetModuleHandle(curModule.ModuleName), 0);
                }

                bool gameHasStarted = false;
                int notRunningCount = 0;

                Timer exitTimer = new Timer();
                exitTimer.Interval = 2000;
                exitTimer.Tick += (s, e) =>
                {
                    Process[] procs = Process.GetProcesses();
                    bool running = false;
                    foreach (var pr in procs)
                    {
                        string pn = pr.ProcessName.ToLower();
                        if (pn.Contains("poptb") || pn.Contains("d3dpoptb"))
                        {
                            running = true;
                            break;
                        }
                    }

                    if (running)
                    {
                        gameHasStarted = true;
                        notRunningCount = 0;
                    }
                    else
                    {
                        notRunningCount++;
                        if (gameHasStarted && notRunningCount >= 2)
                        {
                            Application.Exit();
                        }
                        if (!gameHasStarted && notRunningCount >= 15)
                        {
                            Application.Exit();
                        }
                    }
                };
                exitTimer.Start();

                Application.Run();

                if (_hookID != IntPtr.Zero)
                {
                    UnhookWindowsHookEx(_hookID);
                }
            }
        }
    }
}
