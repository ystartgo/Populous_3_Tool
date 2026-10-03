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

3. **原生 DirectInput 現代 RTS 滑鼠控制 (`dinput.dll` 轉發層 + 引擎按鍵對應攔截)**
   - **符合現代標準 RTS 操作習慣（左鍵選取/取消、右鍵移動）**：
     - **左鍵（選取/框選/點空地取消）**：
       - **點擊信徒選取、按住拖曳金光圈框選信徒**。
       - **點擊建築、法術圖示與 UI 選單**。
       - **點擊空地取消選取（Deselect）**：選取信徒後，點擊空地立即取消選取，**左鍵絕不觸發任何移動跑位**！
     - **右鍵（角色移動/進攻）**：
       - **點擊地面下達移動/進攻指令**：選取信徒後，右鍵點擊目標地面，信徒立即跑步就位！
       - **保留原版旋轉與取消**：按住 `Shift + 右鍵` 或在未選取信徒時右鍵拖曳，維持原版旋轉視角與取消功能；亦可隨時按 `ESC` 取消選取。
     - **滑鼠中鍵（滾輪按下拖曳）**：直接旋轉 3D 視角，如同現代 3D 戰略遊戲。
   - **底層技術原理解析（精準引擎按鍵對應轉譯）**：
     - 《上帝也瘋狂 3》在 1998 年採用了 DirectX DirectInput 5.0 架構，透過 `GUID_SysMouse` 建立滑鼠裝置，在主迴圈直接以 `GetDeviceData` 讀取硬體隊列，標準 User32 / AHK 腳本完全無效。
     - 原版引擎將滑鼠按鍵轉換為動作的機制，是透過輸入模式旗標（`VA 0x0067362F`，選取信徒時為 `0x0D`）與按鍵對應查詢函式（`VA 0x004172A0 (Lookup)`）。
     - 本專案 `dinput.dll` 代理層在引擎載入時精準攔截 `VA 0x004172A0`：
       - 當處於信徒選取模式（`0x0D`）且為放開（Release，`type 4`）事件時：
         - 玩家釋放**左鍵**（`0xF0`）時，即時映射為原版**右鍵動作（Action 108：取消選取 Deselect）**，徹底消除左鍵移動！
         - 玩家釋放**右鍵**（`0xF1`）時，即時映射為原版**左鍵動作（Action 109：下達移動指令 Move）**，實現右鍵專責移動！
       - 在未選取信徒時或框選拖曳時，完全保留原版左鍵選取與右鍵旋轉，**不重對應 DirectInput 底層實體按鍵**，因此視角旋轉、法術施放、雙游標防護 100% 完美穩定。
   - **Alt-Tab 視窗切換滑鼠自動復原 (Alt-Tab Auto-Recovery)**：
     - **歷史問題**：1998 原版引擎在 Alt-Tab 切出遊戲時，會將內部視窗焦點旗標（`0x0059D828`）歸零並釋放（Unacquire）滑鼠裝置；在現代 Win10/Win11 與封裝層下切回遊戲時，因未收到或遺失啟用訊息，導致引擎誤以為仍處於背景，滑鼠永久無法控制。
     - **底層修復**：代理層精準攔截焦點查詢核心（`VA 0x005015C0`），每當遊戲切回前台時，自動呼叫輸入裝置管理器（`VA 0x0051CC40`）重新獲取（Reacquire）滑鼠與鍵盤裝置，並自動重設活動旗標，**徹底解決 Alt-Tab 切回後滑鼠死鎖的問題**！
   - **零背景行程、跨啟動相容**：
     - 無需在背景常駐任何第三方程式（已徹底淘汰舊版 `PopulousMouseHelper.exe`）。
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

6. **遊戲內建密技與 72 項除錯模式指南 (In-Game Cheats & Debug Mode Guide)**
   - **重要前置條件（必看）**：
     - **Windows 輸入法切換**：在按下密技組合鍵與輸入文字前，務必將輸入法切換為「純英文模式」（ENG 或按 Win + 空白鍵 切換）。若處於微軟中文輸入法（注音、拼音等），DirectInput 會攔截按鍵導致密碼無效。
     - **筆記型電腦鍵盤**：若筆電預設將 F1～F12 設為螢幕亮度/音量控制，請務必搭配 Fn 鍵（例如按 Fn + Tab + F11）。
   - **啟動密技控制台**：
     1. 進入戰役或單人遊戲關卡中。
     2. 同時按下 **Tab + F11** 開啟遊戲內控制台（對話框）。
     3. 鍵入密碼 **BYRNE**（或小寫 yrne），然後按下 **Enter** 鍵。
     4. 畫面出現提示即表示密技模式已成功啟用！
   - **密技快捷鍵列表**：
     | 快捷鍵組合 | 效果功能說明 |
     | :--- | :--- |
     | **Tab + F1** | 加速法力回充（Faster mana regen，需持續連點按住） |
     | **Tab + F2** | 法術無消耗／無限施法（Free spells） |
     | **Tab + F3** | 立即解鎖並獲得所有法術（Receive all spells） |
     | **Tab + F4** | 立即解鎖並獲得所有建築藍圖（Receive all buildings） |
     | **Tab + F5** | 立即回滿魔法點數（Receive max mana） |
     | **Shift + =** | 遊戲進行速度倍速加快（最高可達 33 倍速） |
   - **全 72 項除錯模式 (Debug Mode Cheats)**：
     - **操作方式**：啟用密碼後，長按鍵盤英文字母 **O** 不放，同時按 **方向鍵上下 (↑ / ↓)** 捲動瀏覽除錯變數項目；按 **方向鍵左右 (← / →)** 調整數值開關。
     - **完整變數清單**：
       1. Messages Off (關閉系統訊息)
       2. Auto Quick Save (自動快速存檔)
       3. Sound (音效開關)
       4. Music Type (音樂播放類型)
       5. Sync Checking (連線同步檢查)
       6. No Draw (停止畫面繪製)
       7. No Sprites Draw (停止精靈 Sprite 繪製)
       8. Pause on out of sync (同步失敗時暫停)
       9. No Texture Mapping (關閉貼圖映射)
       10. No poly draw (關閉多邊形繪製)
       11. No Objects Draw (關閉物件繪製)
       12. Plan View Hide Enemy People (地圖俯視圖隱藏敵方人口)
       13. Plan View Hide Enemy buildings (地圖俯視圖隱藏敵方建築)
       14. Scroll Momentum (畫面捲動慣性)
       15. Scroll momentum amount (捲動慣性強度數值)
       16. Texture Map Size (材質貼圖尺寸)
       17. Ambient Light (環境光源強度)
       18. Point Lights (點光源開關)
       19. Ambient Shadows (環境陰影開關)
       20. Point Shadows (點光源陰影開關)
       21. Sky On (天空渲染開關)
       22. Hires Textures Off (關閉高解析材質)
       23. Use 32x32 Hires Textures (使用 32x32 高解析貼圖)
       24. Footsteps (腳印痕跡開關)
       25. Animating Water (水面動態效果)
       26. No Formations (關閉隊形演算)
       27. Show Formation Points (顯示隊形定位點)
       28. Frame Limit min ticks (幀率限制最小 Tick 數)
       29. Computer Player off (關閉電腦 AI 玩家運算)
       30. People panel (人口狀態面板)
       31. no jan navigation (關閉 Jan 路徑導航)
       32. Full Map Sync Check (全地圖同步檢查)
       33. Show Jav Points (顯示 Jav 路徑導航節點)
       34. Computer jnav work path len (電腦 AI 路徑運算長度)
       35. Human jav work path len (人類玩家路徑運算長度)
       36. Computer Jav calls per frame (電腦每幀導航計算次數)
       37. Human jnav calls per frame (人類每幀導航計算次數)
       38. Auto Guarding off (關閉信徒自動巡邏防守)
       39. Blow damage off (關閉擊飛/衝擊傷害)
       40. Camera Zoom on (鏡頭縮放功能開啟)
       41. Sound Volume (音效音量大小)
       42. Music Volume (音樂音量大小)
       43. Sea On (海洋水體渲染開關)
       44. Island Level (島嶼海平面高度調整)
       45. CD Track (CD 音軌選首切換)
       46. Local Drag Select Off (關閉本機框選拖曳選取)
       47. Jnav max count (導航節點最大上限數)
       48. Fog of war (戰爭迷霧開關)
       49. Auto use vehicles (信徒自動使用船隻/載具)
       50. Maintain minimum population (維持最低人口底限機制)
       51. Continuous raise/lower (地形連續升降模式)
       52. Allow cursor snap (允許滑鼠游標自動吸附格線)
       53. Game turns per second (遊戲每秒邏輯運算回合計時)
       54. Draw Turns per second (畫面每秒渲染回合數)
       55. Show flat areas darkened (平坦可建造地塊以暗色高亮顯示)
       56. Scaling spires always (永久縮放尖頂建築物件)
       57. Show attack areas (顯示法術與戰鬥攻擊範圍判定)
       58. Show wood search data (顯示信徒砍樹尋找木材數據)
       59. Check mapwho integrity (檢查地圖網格空間分割完整性)
       60. Use buildings entrance alt (使用建築物備用出入口)
       61. Ok (狀態確認)
       62. Tooltips (滑鼠懸停提示資訊開關)
       63. Auto Camera adjust (鏡頭視角自動跟隨調整)
       64. Lens Flare (太陽鏡頭眩光特效)
       65. Panel sound effects off (關閉操作面板點擊音效)
       66. Human shaman omnipresence (人類巫師全知全能/無所不在)
       67. Owned Target select (自身目標指定選取)
       68. Autocast spell (法術自動施放開關)
       69. Scrolling tool tips (捲動提示條訊息)
       70. Flip Rotation (鏡頭旋轉方向反轉翻轉)
       71. Auto Deselect (自動取消選取功能)
       72. Swap Rotate/Move (鏡頭旋轉與移動鍵位互換)

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

#### 滑鼠操作模式切換與還原 (Mouse Control Switching & Restoring)
- **【一鍵還原原版模式】**：若遇雙游標或習慣原版操控，雙擊執行 還原滑鼠設定(原版模式).bat，將安全停用代理層並恢復原版 1998 操控（左鍵選取兼移動，右鍵旋轉/取消，單一游標零衝突）。
- **【一鍵切換現代模式】**：雙擊執行 套用現代滑鼠設定(右鍵移動模式).bat，即可重新啟用右鍵移動小人功能。
- **【圖形化介面自訂】**：雙擊執行 遊戲畫面與字體設定.bat，可自由調整解析度、比例、字體陰影，並隨時點擊「還原原版滑鼠」按鈕。

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

3. **Native DirectInput Modern RTS Controls (`dinput.dll` Proxy + Key Binding Interception)**
   - **Modern RTS Control Scheme (Left-Click Select/Deselect, Right-Click Move)**:
     - **Left-Click (Selection / Box-Select / Click Ground to Deselect)**:
       - **Click followers to select, click-drag to expand golden selection box**.
       - **Click spells, building plans, and UI buttons**.
       - **Click empty ground to Deselect**: When followers are selected, clicking empty terrain instantly deselects them. **Left-Click NEVER triggers ground movement!**
     - **Right-Click (Follower Movement / Orders)**:
       - **Click ground to move**: With followers selected, right-clicking target terrain commands followers to run to that destination!
       - **Preserved Original Functions**: Hold `Shift + Right Click` or right-click-drag when no units are selected to rotate the 3D camera / cancel actions; or press `ESC` anytime to deselect.
     - **Middle-Click (Scroll Wheel Drag)**: Rotates the 3D camera smoothly, just like modern 3D RTS titles.
   - **Technical Root Cause & Clean Engine Hook Architecture**:
     - *Populous: The Beginning* (1998) processes hardware mouse events via DirectInput 5.0 into an internal input mode byte (`VA 0x0067362F`, which equals `0x0D` when followers are selected) and looks up resulting actions via an internal context-sensitive binding lookup table (`VA 0x004172A0 (Lookup)`).
     - In the original engine under mode `0x0D`, Left-Click release (`0xF0`, `type 4`) maps to Action 109 (Move), while Right-Click release (`0xF1`, `type 4`) maps to Action 108 (Deselect).
     - Our native **`dinput.dll` proxy** hooks `VA 0x004172A0` cleanly:
       - In selection mode (`0x0D`), Left-Click release is dynamically translated to Action 108 (Deselect), eliminating false movement orders on Left-Click.
       - Right-Click release is dynamically translated to Action 109 (Move), issuing unit movement commands directly.
       - Raw DirectInput physical buttons are **NOT** swapped or modified, completely preventing double-cursor glitches, camera rotation breakage, and spell targeting conflicts.
   - **Alt-Tab Window Switching & Mouse Input Auto-Recovery**:
     - **Root Cause**: When Alt-Tabbing away, the 1998 engine clears its internal focus flag (`0x0059D828`) and unacquires DirectInput devices. Upon returning in Windows 10/11, missing or swallowed activation messages left the engine believing it was still in the background, causing complete mouse loss.
     - **Engine Hook Fix**: Our proxy intercepts the focus query at `VA 0x005015C0`. Whenever the game process regains foreground status, it immediately re-invokes the input manager (`VA 0x0051CC40`) to reacquire all mouse/keyboard devices and sets the active flag back to 1, **completely eliminating the Alt-Tab mouse freeze**!
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

6. **In-Game Cheats & Debug Mode Guide**
   - **Important Prerequisites (Must Read)**:
     - **Windows Keyboard Layout**: Always switch Windows IME to English (ENG or press Win + Space). Chinese or third-party IMEs will intercept keystrokes before DirectInput can register them.
     - **Laptop Keyboards**: If your laptop functions map F1–F12 to multimedia/brightness keys by default, hold Fn while pressing the shortcuts (e.g., Fn + Tab + F11).
   - **Activating Cheat Console**:
     1. Enter any single-player campaign or skirmish game.
     2. Press **Tab + F11** simultaneously to open the chat/console prompt.
     3. Type **BYRNE** (case-insensitive) and press **Enter**.
     4. A confirmation indicates cheat mode is active.
   - **Cheat Shortcut Combinations**:
     | Shortcut | Effect |
     | :--- | :--- |
     | **Tab + F1** | Faster mana regeneration (press continuously) |
     | **Tab + F2** | Free spells (cast without mana cost) |
     | **Tab + F3** | Receive and unlock all spells |
     | **Tab + F4** | Receive and unlock all building plans |
     | **Tab + F5** | Instant max mana refill |
     | **Shift + =** | Accelerate game simulation speed (up to 33x) |
   - **Complete 72 Debug Mode Cheats**:
     - **Usage**: Once cheats are activated, hold the letter **O** key and use **Arrow Up / Down (↑ / ↓)** to cycle through variables; use **Arrow Left / Right (← / →)** to toggle values.
     - **Variables List (1–72)**:
       1. Messages Off, 2. Auto Quick Save, 3. Sound, 4. Music Type, 5. Sync Checking, 6. No Draw, 7. No Sprites Draw, 8. Pause on out of sync, 9. No Texture Mapping, 10. No poly draw, 11. No Objects Draw, 12. Plan View Hide Enemy People, 13. Plan View Hide Enemy buildings, 14. Scroll Momentum, 15. Scroll momentum amount, 16. Texture Map Size, 17. Ambient Light, 18. Point Lights, 19. Ambient Shadows, 20. Point Shadows, 21. Sky On, 22. Hires Textures Off, 23. Use 32x32 Hires Textures, 24. Footsteps, 25. Animating Water, 26. No Formations, 27. Show Formation Points, 28. Frame Limit min ticks, 29. Computer Player off, 30. People panel, 31. no jan navigation, 32. Full Map Sync Check, 33. Show Jav Points, 34. Computer jnav work path len, 35. Human jav work path len, 36. Computer Jav calls per frame, 37. Human jnav calls per frame, 38. Auto Guarding off, 39. Blow damage off, 40. Camera Zoom on, 41. Sound Volume, 42. Music Volume, 43. Sea On, 44. Island Level, 45. CD Track, 46. Local Drag Select Off, 47. Jnav max count, 48. Fog of war, 49. Auto use vehicles, 50. Maintain minimum population, 51. Continuous raise/lower, 52. Allow cursor snap, 53. Game turns per second, 54. Draw Turns per second, 55. Show flat areas darkened, 56. Scaling spires always, 57. Show attack areas, 58. Show wood search data, 59. Check mapwho integrity, 60. Use buildings entrance alt, 61. Ok, 62. Tooltips, 63. Auto Camera adjust, 64. Lens Flare, 65. Panel sound effects off, 66. Human shaman omnipresence, 67. Owned Target select, 68. Autocast spell, 69. Scrolling tool tips, 70. Flip Rotation, 71. Auto Deselect, 72. Swap Rotate/Move

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

#### Mouse Control Switching & Restoring
- **One-Click Restore to Classic Mode**: Run 還原滑鼠設定(原版模式).bat to safely disable the proxy layer and revert to 100% vanilla 1998 mouse controls (Left-Click Select/Move, Right-Click Rotate/Cancel, guaranteed single cursor).
- **One-Click Enable Modern RTS Mode**: Run 套用現代滑鼠設定(右鍵移動模式).bat to re-enable Right-Click movement.
- **GUI Settings Tool**: Run 遊戲畫面與字體設定.bat to manage all settings or click "還原原版滑鼠" anytime.

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