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
   - **Windows 原生圖形化視窗**：免進遊戲選單，直覺以單選圓鈕調整所有參數。
   - **4:3 經典比例維持**：保持正方形像素比例（推薦），避免 16:9 水平拉伸導致中文字元被壓扁 33%。
   - **文字清晰化模式**：消除原版中文字體背後厚重的 `+2, +2` 像素黑底重影，讓 16×16 細明體點陣字清晰銳利、好讀不累眼。
   - **3D 渲染解析度安全切換**：在外部安全預設為 1024×768（最高畫質）/ 800×600 / 640×480，解決遊戲內選單即時更換解析度導致的崩潰。

3. **現代 RTS 滑鼠右鍵移動小人 (`PopulousMouseHelper.exe` / `PopulousMouseHelper.cs`)**
   - **符合現代操作習慣**：
     - **左鍵**：點擊選取、拉框框選信徒、施放法術、點擊介面。
     - **右鍵**：**點擊地面直接下達移動/進攻指令**（小人直接跑步就位，不再被取消選取）！
     - **取消選取**：按鍵盤 `ESC`、`空白鍵` 或 `Shift + 右鍵`。
   - **輕量與無干擾**：以 C# 編寫的低階滑鼠掛鉤，記憶體佔用小於 5MB，CPU 0.00%；僅在遊戲視窗處於最上層時生效，遊戲關閉後自動安全退出。

4. **遊戲內換解析度閃退原因說明**
   - **崩潰原因**：1998 年的 DirectX 5/6 引擎在切換解析度時，會嘗試即時銷毀並重建 DirectDraw 表面；在 Windows 10/11 的現代顯卡驅動下會引發記憶體釋放違規（`ntdll.dll 0xc0000005`）。
   - **對策**：使用本工具直接在外部預先設定為 `1024x768`（最高畫質），進入遊戲後請勿在選單中切換解析度。

---

### 📂 檔案編碼與格式規範 (File Encodings & Formats)

為確保在現代 Windows 系統與 PowerShell 環境下穩定執行，本專案所有檔案均嚴格遵循以下編碼規範：

| 檔案類型 / 副檔名 | 換行格式 (EOL) | 字元編碼 (Encoding) | 說明 |
| :--- | :---: | :---: | :--- |
| **`.bat` (批次檔)** | `CRLF (\r\n)` | `UTF-8 (無 BOM)` | 開頭均包含 `chcp 65001 >nul`，確保中文字元在 CMD 中正常執行與顯示。 |
| **`.ps1` (PowerShell)** | `CRLF (\r\n)` | `UTF-8 with BOM (utf-8-sig)` | **重要**：Windows PowerShell 5.1 解析無 BOM 的 UTF-8 檔案時會預設當作系統 ANSI (CP950) 解析，導致多位元組中文字元引發語法錯誤閃退；因此必須強制採用 **帶 BOM 的 UTF-8**。 |
| **`.cs` (C# 原始碼)** | `CRLF (\r\n)` | `UTF-8` | 相容 .NET Framework `csc.exe` 編譯器。 |
| **`.ini` / `.cfg` (設定檔)** | `CRLF (\r\n)` | `UTF-8` | `ddraw.ini` 與設定設定檔，供 DirectDraw 轉換層讀取。 |

---

### 🚀 安裝與使用指南

1. **套用繁體中文補丁**：
   - 進入 `Steam中文語系補丁` 資料夾，以管理員身分執行 `套用中文化到Steam(UTF8).bat`。
2. **調整畫面與字體外觀**：
   - 雙擊執行 `遊戲畫面與字體設定.bat`，即可開啟圖形化設定視窗，自由選擇比例、解析度與字體陰影。
3. **體驗現代 RTS 右鍵移動**：
   - 雙擊執行 `啟動上帝也瘋狂(右鍵移動版).bat`，遊戲將自動掛載右鍵移動輔助並直接進入遊戲！

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

3. **Modern RTS Right-Click Unit Movement (`PopulousMouseHelper.exe` / `PopulousMouseHelper.cs`)**
   - **Modern RTS Control Scheme**:
     - **Left Click**: Select single follower, drag-box select units, cast spells, click UI.
     - **Right Click**: **Click on ground to issue movement/attack commands** (followers immediately run to the target location instead of being deselected)!
     - **Deselect**: Press `ESC`, `Space`, or `Shift + Right Click`.
   - **Zero Overhead**: Written in C# using low-level mouse hooks (`WH_MOUSE_LL`). Uses < 5MB RAM and 0.00% CPU. Only active when the Populous window is focused, and automatically terminates when the game exits.

4. **In-Game Resolution Crash Explanation**
   - **Root Cause**: The 1998 DirectX 5/6 engine attempts to instantly destroy and recreate DirectDraw surfaces mid-frame when clicking the in-game resolution slider, triggering heap corruption in `ntdll.dll (0xc0000005)` on Windows 10/11 WDDM drivers.
   - **Solution**: Pre-configure the resolution to `1024x768` using the provided Settings GUI tool and avoid clicking the in-game resolution arrows.

---

### 📂 File Formats and Encoding Specifications

To guarantee reliable execution across modern Windows systems and PowerShell 5.1+:

| File Type / Extension | Line Endings (EOL) | Character Encoding | Notes |
| :--- | :---: | :---: | :--- |
| **`.bat` (Batch Scripts)** | `CRLF (\r\n)` | `UTF-8 (without BOM)` | Includes `chcp 65001 >nul` to ensure proper UTF-8 console output. |
| **`.ps1` (PowerShell)** | `CRLF (\r\n)` | `UTF-8 with BOM (utf-8-sig)` | **Critical**: Windows PowerShell 5.1 parses non-BOM UTF-8 files as system ANSI (e.g. CP950/CP1252), causing Chinese characters to break script syntax and flash-crash. Must use UTF-8 with BOM. |
| **`.cs` (C# Source)** | `CRLF (\r\n)` | `UTF-8` | Fully compatible with .NET Framework `csc.exe` v4.x. |
| **`.ini` / `.cfg` (Configs)** | `CRLF (\r\n)` | `UTF-8` | For DirectDraw wrapper (`ddraw.ini`) and helper configuration. |

---

### 🚀 Usage Instructions

1. **Apply Traditional Chinese Patch**:
   - Open `Steam中文語系補丁` and run `套用中文化到Steam(UTF8).bat` as Administrator.
2. **Configure Display & Font Quality**:
   - Double-click `遊戲畫面與字體設定.bat` to launch the GUI configuration tool.
3. **Play with Modern Right-Click Controls**:
   - Double-click `啟動上帝也瘋狂(右鍵移動版).bat` to launch the game with modern RTS right-click movement enabled!

---

### 📄 License

This project is licensed under the [GNU General Public License v3.0](LICENSE).
Populous: The Beginning is © Electronic Arts / Bullfrog Productions.