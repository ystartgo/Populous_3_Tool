# Populous 3 Tool (《上帝也瘋狂：開天闢地》現代化與繁體中文化工具箱)

[English](#english) | [繁體中文](#繁體中文)

---

<a name="繁體中文"></a>
## 繁體中文說明

本專案為經典即時戰略神作《上帝也瘋狂：開天闢地》（*Populous: The Beginning*，又稱《Populous 3》）在現代 Windows 10 / 11 系統（Steam / GOG / Multiverse 平台）上的完整優化、修復與繁體中文化整合工具箱。

### 🌟 主要功能與修復

1. **Steam / Multiverse 一鍵繁體中文化補丁**
   - **解決經典亂碼問題**：修復 1998 年牛蛙（Bullfrog）引擎將註冊表數值 `9` 誤判為 Windows LCID `LANG_ENGLISH` 的歷史問題，徹底解決主選單按鈕顯示為 `<<<`、`© © © ©`、`Ç Ç Ç Ç` 等西文精靈符號的亂碼現象。
   - **中文字型完整注入**：同步注入完整的 16×16 與 24×24 Big5 中文字型檔（`B5fnt16.bit`、`B5fnt16.idx`、`B5Fnt24.bit`）以及繁體中文遊戲標誌材質（`FELGsdTC.spr`、`felgsptc.spr`）。
   - **相容性支援**：原生支援 Steam 啟動與社群 Multiverse Launcher（全戰役槽位 GameData 0~2 完整注入）。

2. **遊戲畫面與字體外觀 GUI 設定工具 (`遊戲畫面與字體設定.bat` / `Populous_Settings_GUI.ps1`)**
   - **Windows 原生圖形化視窗**：免進遊戲選單，直覺以單選圓鈕調整所有畫面與操作參數。
   - **4:3 經典比例維持**：保持正方形像素比例（推薦），避免 16:9 水平拉伸導致中文字元被壓扁 33%。
   - **文字清晰化模式**：消除原版中文字體背後厚重的 `+2, +2` 像素黑底重影，讓 16×16 細明體點陣字清晰銳利、好讀不累眼。
   - **3D 渲染解析度安全切換**：在外部安全預設為 1024×768（最高畫質）/ 800×600 / 640×480，解決遊戲內選單即時更換解析度導致的崩潰。
   - **滑鼠控制一鍵切換**：可隨時在「現代 RTS 模式」與「1998 原版模式」之間切換。

3. **原生 DirectInput 現代 RTS 滑鼠右鍵移動小人 (`dinput.dll` 轉發層)**
   - **符合現代操作習慣**：
     - **左鍵**：點選單一信徒、拉框框選單位、施放法術、點擊 UI。
     - **右鍵**：**點擊地面直接下達移動/進攻指令**（小人直接跑步就位，不再被取消選取）！
     - **取消選取 / 查詢**：按鍵盤 `ESC`、`空白鍵`，或按住 `Shift + 右鍵`（保留原版右鍵功能）。
   - **底層技術原理解析（為什麼外部腳本無效）**：
     - 《上帝也瘋狂 3》在 1998 年採用了 DirectX DirectInput 5.0 架構，透過 `GUID_SysMouse` 建立滑鼠裝置，並在主迴圈直接以 `GetDeviceData` 讀取硬體驅動層的緩衝事件隊列。
     - 傳統外部工具（如 AutoHotkey、`mouse_event`、`SendInput`、Windows `WH_MOUSE_LL` 鉤子）僅能注入 User32 訊息隊列，完全被 DirectInput 底層驅動繞過，因此外部點擊完全無效。
     - 本專案採用 **DirectInput 原生轉發代理（`dinput.dll`）**，直接攔截 `IDirectInputDeviceA::GetDeviceData` 與 `GetDeviceState`，在引擎讀取前將右鍵代碼（`DIMOFS_BUTTON1`）即時轉譯為左鍵移動代碼（`DIMOFS_BUTTON0`）。
   - **零背景行程、跨啟動相容**：
     - 無需在背景常駐任何第三方程式（已淘汰舊版 `PopulousMouseHelper.exe`）。
     - 無論直接從 **Steam 客戶端點擊「開始遊戲」**、點擊桌面捷徑或執行檔，均能 100% 自動生效！

4. **遊戲內換解析度閃退原因說明**
   - **崩潰原因**：1998 年的 DirectX 5/6 引擎在切換解析度時，會嘗試即時銷毀並重建 DirectDraw 表面；在 Windows 10/11 的現代顯卡驅動下會引發記憶體釋放違規（`ntdll.dll 0xc0000005`）。
   - **對策**：使用本工具直接在外部預先設定為 `1024x768`（最高畫質），進入遊戲後請勿在選單中切換解析度。

5. **社群旗艦級擴展：Multiverse Launcher (強烈推薦)**
   - **官方網站**：[Multiverse Launcher 官方專頁](https://thebeginning.uk/multiverse/)
   - **什麼是 Multiverse Launcher？**
     - Multiverse 是全球《上帝也瘋狂 3》社群（The Beginning / PopRE）公認最具代表性的現代擴充客戶端。
     - **自訂戰役模組庫**：整合無數社群自製戰役（如著名的 *Multiverse*、*Ascension*、*The Beginning: Enhanced Edition* 等），無需覆蓋原始遊戲檔案即可自由切換遊玩。
     - **全高清（Full HD）介面**：修復現代寬螢幕下的行星太陽系軌道與主選單畫面。
     - **進階機制擴展**：透過 DLL 注入支援新法術、新建築、火勇士推力（Push Effect）與各項平衡修正。
     - **多人對戰大廳**：內建社群連線大廳與觀戰系統。
   - **與本專案的完美相容性**：
     - 本工具包的繁體中文補丁已直接適配 Multiverse 的戰役結構（自動注入 `Multiverse\GameData\0~2`）。
     - `dinput.dll` 原生現代 RTS 滑鼠操作亦全面支援 Multiverse 核心進程（`popTBM.exe`）。

---

### 📂 檔案編碼與格式規範 (File Encodings & Formats)

為確保在現代 Windows 系統與 PowerShell 環境下穩定執行，本專案所有檔案均嚴格遵循以下編碼規範：

| 檔案類型 / 副檔名 | 換行格式 (EOL) | 字元編碼 (Encoding) | 說明 |
| :--- | :---: | :---: | :--- |
| **`.bat` (批次檔)** | `CRLF (\r\n)` | `UTF-8 (無 BOM)` | 確保批次檔在 CMD / Windows 命令提示字元中正常解析。 |
| **`.ps1` (PowerShell)** | `CRLF (\r\n)` | `UTF-8 with BOM (utf-8-sig)` | **重要**：Windows PowerShell 5.1 解析無 BOM 的 UTF-8 檔案時會預設當作系統 ANSI (CP950) 解析，導致多位元組中文字元引發語法錯誤閃退；因此必須強制採用 **帶 BOM 的 UTF-8**。 |
| **`.ini` / `.cfg` (設定檔)** | `CRLF (\r\n)` | `UTF-8` | `ddraw.ini` 與 `populous_mouse.ini`，供轉發層與 GUI 雙向讀寫。 |
| **`.cpp` / `.def` (原始碼)** | `CRLF (\r\n)` | `UTF-8` | DirectInput 代理層原始碼與模組定義檔，支援 MinGW 32-bit (`i686-w64-mingw32-g++`) 編譯。 |
| **`.dll` (動態連結函式庫)** | N/A (二進位) | x86 (32-bit PE) | 編譯完成的 32 位元 DirectInput 代理庫。 |
| **`.md` (文件)** | `LF (\n)` | `UTF-8` | GitHub 標準 Markdown 格式。 |

---

### 🚀 安裝與使用教學 (Tutorial)

#### 方案 A：使用 Steam 原版體驗
1. **套用繁體中文補丁**：
   - 進入 `Steam中文語系補丁` 資料夾，以管理員身分執行 `套用中文化到Steam(UTF8).bat`。
2. **調整畫面、解析度與操作模式**：
   - 雙擊執行 `遊戲畫面與字體設定.bat`，即可開啟圖形化設定視窗，自由選擇比例、解析度、字體陰影與滑鼠操作習慣。
3. **啟動遊戲**：
   - 設定完成後，直接在 **Steam 客戶端點擊「開始遊戲」** 或點擊桌面捷徑即可！

#### 方案 B：使用社群 Multiverse Launcher（自訂戰役與模組擴展）
1. **下載與安裝**：
   - 前往 [thebeginning.uk/multiverse](https://thebeginning.uk/multiverse/) 下載最新的 Multiverse Launcher 安裝包（或使用遊戲目錄內已附帶的 `MultiverseLauncher.exe`）。
   - 請將 Multiverse Launcher 安裝在《上帝也瘋狂 3》的根目錄下。
2. **執行 Multiverse**：
   - 建議對 `MultiverseLauncher.exe` 點擊右鍵以「系統管理員身分執行」。
3. **選擇與下載戰役**：
   - 在主畫面中可瀏覽社群戰役清單（如 Multiverse、The Beginning: Enhanced 等），點擊即可一鍵下載與安裝。
4. **啟動戰役**：
   - 選擇想遊玩的戰役後點擊 **Play**，遊戲將透過 `popTBM.exe` 啟動。本工具箱的 **繁體中文** 與 **現代右鍵移動** 會自動在 Multiverse 中無縫生效！

---

<a name="english"></a>
## English Documentation

A comprehensive modernization, bug-fix, and Traditional Chinese localization toolkit for *Populous: The Beginning* (*Populous 3*, 1998) running on modern Windows 10 / 11 (Steam, GOG, and Multiverse Launcher platforms).

### 🌟 Key Features & Fixes

1. **Traditional Chinese Localization & Big5 Font Engine Fix**
   - **LCID Registry Fix**: Fixed a classic 1998 Bullfrog engine bug where setting the registry value to `9` caused the game to interpret it as Windows LCID `LANG_ENGLISH`, skipping the Chinese bitmap font loader entirely and rendering Big5 double-byte strings using the Western sprite sheet (resulting in `<<<`, `©©©©`, `ÇÇÇÇ` garbled text).
   - **Complete Chinese Font Assets**: Injects monochrome 16×16 and 24×24 Big5 bitmap fonts (`B5fnt16.bit`, `B5fnt16.idx`, `B5Fnt24.bit`) and Traditional Chinese UI banners (`FELGsdTC.spr`, `felgsptc.spr`).
   - **Multiverse Compatibility**: Full support for both vanilla Steam launch and the community Multiverse Launcher (GameData slots 0–2).

2. **Display & Font Quality Settings GUI Tool (`遊戲畫面與字體設定.bat` / `Populous_Settings_GUI.ps1`)**
   - **Native Windows GUI**: Intuitive Windows Forms interface to customize graphics options without entering the game.
   - **4:3 Aspect Ratio Preservation**: Prevents horizontal 16:9 distortion (which squashes Chinese characters by ~33%), keeping pixels crisp and square.
   - **Clean Font Rendering Mode**: Removes the heavy `+2, +2` pixel drop-shadow pass that caused dense 16×16 Chinese bitmap characters to blur and bleed into neighboring strokes.
   - **Safe 3D Resolution Pre-configuration**: Allows safely selecting 1024×768 (highest quality), 800×600, or 640×480 prior to launching the game.
   - **Mouse Control Scheme Toggle**: Switch freely between Modern RTS controls and the classic 1998 controls.

3. **Native DirectInput Modern RTS Right-Click Movement (`dinput.dll` Proxy)**
   - **Modern RTS Control Scheme**:
     - **Left Click**: Select single follower, drag-box select units, cast spells, click UI.
     - **Right Click**: **Click on ground to issue movement/attack commands** (followers immediately run to the target location instead of being deselected)!
     - **Deselect / Query**: Press `ESC`, `Space`, or hold `Shift + Right Click` (preserves original right-click query/deselect behavior).
   - **Technical Root Cause (Why External Hooks / AHK Failed)**:
     - *Populous: The Beginning* utilizes DirectX DirectInput 5.0 (`GUID_SysMouse`), retrieving raw mouse events directly from the driver buffer via `IDirectInputDeviceA::GetDeviceData`.
     - Standard Windows User32 hooks (`WH_MOUSE_LL`) and simulated inputs (`mouse_event`, `SendInput`, AutoHotkey) only interact with the User32 message queue and are completely bypassed by DirectInput.
     - This project provides a native **DirectInput proxy DLL (`dinput.dll`)** that hooks `GetDeviceData` and `GetDeviceState` from within the process space, translating Right-Click (`DIMOFS_BUTTON1`) into Left-Click (`DIMOFS_BUTTON0`) in real time before the game engine processes it.
   - **Zero Background Processes & Steam Native**:
     - No external background helper processes required (retired legacy `PopulousMouseHelper.exe`).
     - Works seamlessly regardless of launch method: directly via the Steam client "Play" button, desktop shortcuts, or custom launchers.

4. **In-Game Resolution Crash Explanation**
   - **Root Cause**: The 1998 DirectX 5/6 engine attempts to instantly destroy and recreate DirectDraw surfaces mid-frame when clicking the in-game resolution slider, triggering heap corruption in `ntdll.dll (0xc0000005)` on Windows 10/11 WDDM drivers.
   - **Solution**: Pre-configure the resolution to `1024x768` using the provided Settings GUI tool and avoid clicking the in-game resolution arrows.

5. **Community Flagship Expansion: Multiverse Launcher (Highly Recommended)**
   - **Official Website**: [Multiverse Launcher Page](https://thebeginning.uk/multiverse/)
   - **What is the Multiverse Launcher?**
     - Multiverse is the premier community launcher and client for *Populous: The Beginning*, maintained by the active community at The Beginning and PopRE.
     - **Custom Campaigns & Modding**: Play custom user-made campaigns (such as *Multiverse*, *Ascension*, and *Enhanced Edition*) without overwriting game files.
     - **Full HD Menus**: High-definition rendering of the solar system and main menus for modern widescreen displays.
     - **Advanced Gameplay Features**: Injects DLL-level enhancements including new spells, buildings, unit mechanics (e.g. Firewarrior pushback), and physics fixes.
     - **Multiplayer Lobby**: Built-in online matchmaking and spectating system.
   - **Compatibility**: Fully compatible with our Traditional Chinese localization patch and our native `dinput.dll` modern mouse controls!

---

### 📂 File Formats and Encoding Specifications

To guarantee reliable execution across modern Windows systems and PowerShell 5.1+:

| File Type / Extension | Line Endings (EOL) | Character Encoding | Notes |
| :--- | :---: | :---: | :--- |
| **`.bat` (Batch Scripts)** | `CRLF (\r\n)` | `UTF-8 (without BOM)` | Standard batch scripts for CMD / Windows Command Prompt. |
| **`.ps1` (PowerShell)** | `CRLF (\r\n)` | `UTF-8 with BOM (utf-8-sig)` | **Critical**: Windows PowerShell 5.1 parses non-BOM UTF-8 files as system ANSI (e.g. CP950/CP1252), causing Chinese characters to break script syntax and flash-crash. Must use UTF-8 with BOM. |
| **`.ini` / `.cfg` (Configs)** | `CRLF (\r\n)` | `UTF-8` | For DirectDraw wrapper (`ddraw.ini`) and mouse configuration (`populous_mouse.ini`). |
| **`.cpp` / `.def` (Source)** | `CRLF (\r\n)` | `UTF-8` | DirectInput proxy source code and module definition, compiled via MinGW 32-bit (`i686-w64-mingw32-g++`). |
| **`.dll` (Binaries)** | N/A (Binary) | x86 (32-bit PE) | Precompiled 32-bit DirectInput proxy library. |
| **`.md` (Documentation)** | `LF (\n)` | `UTF-8` | GitHub standard Markdown format. |

---

### 🚀 Usage Instructions & Tutorial

#### Option A: Playing via Steam
1. **Apply Traditional Chinese Patch**:
   - Open `Steam中文語系補丁` and run `套用中文化到Steam(UTF8).bat` as Administrator.
2. **Configure Display & Controls**:
   - Double-click `遊戲畫面與字體設定.bat` to launch the GUI configuration tool.
3. **Launch the Game**:
   - Launch directly from Steam ("Play" button) or desktop shortcut!

#### Option B: Playing via Multiverse Launcher (Custom Campaigns & Mods)
1. **Download & Setup**:
   - Download the installer from [thebeginning.uk/multiverse](https://thebeginning.uk/multiverse/) (or run `MultiverseLauncher.exe` directly from the game directory).
   - Ensure the launcher is located inside the root *Populous: The Beginning* folder.
2. **Run as Administrator**:
   - Right-click `MultiverseLauncher.exe` and select "Run as Administrator".
3. **Install Campaigns**:
   - Browse the campaign list in the launcher and download your desired campaigns.
4. **Launch & Play**:
   - Select a campaign and click **Play**. It will launch via `popTBM.exe` with our Traditional Chinese patch and modern right-click controls active!

---

### 📄 License

This project is licensed under the [GNU General Public License v3.0](LICENSE).  
Populous: The Beginning is © Electronic Arts / Bullfrog Productions.