  # 先看這裡:推薦用「合併版」DOOM.uf2

  **推薦做法**:到
  [Releases](../../releases/tag/retro-loader-v1) 下載 `DOOM.uf2` ——
  **韌體與地圖檔已經包成同一份檔案**,拖進 `RPI-RP2` 一次就能玩,
  不需要 `picotool`,也不需要 SD 卡或載入器。細節見下方
  「另有一份「拖一次就能玩」的版本」一節。

  **Recommended**: grab `DOOM.uf2` from
  [Releases](../../releases/tag/retro-loader-v1) — firmware and map data are
  **bundled into a single file**. Drag it onto `RPI-RP2` once and play. No
  `picotool`, no SD card, no loader needed.

  本資料夾裡的 `doom_tiny_*.uf2` 是**舊的分離式版本**:要先拖韌體、再用
  `picotool` 另外燒地圖檔,而且**不含**後來把預設音量調大的改動。
  除非你有特別理由(例如要用 ST7789 螢幕),否則不建議走這條路。

  The `doom_tiny_*.uf2` files in this folder are the **older two-step split
  build**: flash the firmware, then flash the map data separately with
  `picotool`. They also do **not** include the later default-volume bump.
  Use them only if you have a specific reason (e.g. an ST7789 panel).

  ---

  ## 舊的分離式安裝方法(不推薦)

  首先將遊戲機設置為usb上傳模式，然後根據版本選擇對應的 .uf2 檔案拖拉至遊戲機：

   1. 標準版 (Standard): doom_tiny.uf2
   2. 音效修正版 (Sound Fix): doom_tiny_ILI9341_sound_fix.uf2 (分離式版本中較佳的一份，包含 2x 超取樣音效、螢幕亮度提升與穩定性優化)

  使用指令上傳地圖檔

  請注意！根據您選擇的韌體版本，地圖檔的燒錄位址（-o 參數）有所不同：

   - 若使用標準版 (Standard):
    `picotool load -v -t bin ../doom1.whx -o 0x10040000`

   - 若使用音效修正版 (Sound Fix):
    `picotool load -v -t bin ../doom1.whx -o 0x10042000` (重要：因程式體積較大，位址需後移以避免重疊當機)

  重開機後即可遊玩

  ---

  ## 另有一份「拖一次就能玩」的版本(在 Releases,不在這個資料夾)

  上面那幾份都要分兩步:先拖韌體,再用 `picotool` 燒地圖檔。如果嫌麻煩,
  [Releases](../../releases/tag/retro-loader-v1) 有一份 `DOOM.uf2`,
  **韌體與地圖檔已經包在同一份檔案裡**,拖一次就好。

  那一份**兩種用法都成立**:

  1. **獨立燒錄** — 直接拖進 `RPI-RP2`,開機就進 DOOM,不需要 SD 卡,
     也不需要載入器
  2. **從選單載入** — 放進 SD 卡根目錄,由
     [rp2040-retro-loader](https://github.com/pondahai/rp2040-retro-loader)
     的圖形選單挑

  ⚠️ 那一份**不能**搭配上面那些 `picotool load -o ...` 指令,位址對不上
  (它的韌體在 `0x10004000`、地圖檔在 `0x10046000`)。它自己就是完整的。

  **2026-08-16 起那一份的預設音量調大了**(音效與音樂的預設值 8 -> 15,
  約 +5.5dB)。DOOM 的出廠預設只有一半音量,而這個韌體沒有地方存設定,
  選單裡調的音量每次開機都會掉回預設值 —— 所以是直接改編譯期的初始值。
  上面那幾份資料夾裡的韌體**未包含**這個改動,仍是原本的音量。

  因為檔案有 4.2MB,放 Releases 而不放進版控,免得 repo 每次重編都胖一圈。

  ### 檔案完整性(2026-08-25 拆檔驗證)

  曾經有過「拖拉燒錄失敗」的疑慮,原因通常是合併版由兩份 UF2 直接接起來 ——
  每一份都帶著自己的 `blockNo` 0..N-1 與自己的 `numBlocks`,而 RP2040 的
  USB 磁碟端就是靠這兩個欄位判斷收完了沒,看到第二份重新從 0 開始就會提早
  重開機:韌體進去了、地圖沒進去。

  目前 Releases 這份**不是接出來的,是重新產生的一份完整 UF2**,實際拆檔量過:

  - 8153 個 block,`blockNo` 從 0 連號到 8152,每一塊的 `numBlocks` 都是 8153
  - family ID 統一為 `0xE48BFF56`(RP2040),flags 一致
  - 位址 `0x10000000`–`0x101FD900` **完全連續,中間沒有任何跳號**
  - 空隙有填滿:韌體結尾 `0x10044448` 到 WAD 起點 `0x10046000` 之間的
    7096 bytes 以 `0xFF`/`0x00` 填充,trampoline 到 `0x10004000` 之間亦同

  版面配置:trampoline `0x10000000`(boot2 + 向量表)、DOOM 韌體
  `0x10004000`、地圖資料 `0x10046000`。前 16KB 那顆 trampoline 就是
  standalone 拖拉能開機的原因 —— 韌體雖然 link 在 `0x10004000`,bootrom
  仍能在 `0x10000000` 找到有效映像並跳進去。

  ---

  ## A one-drag version is available (in Releases, not in this folder)

  All the files above need two steps: drag the firmware, then flash the map data
  with `picotool`. If you'd rather not, [Releases](../../releases/tag/retro-loader-v1)
  has a `DOOM.uf2` that **bundles the firmware and the map data into one file** —
  drag it once and you're done.

  That file works **both ways**:

  1. **Standalone** — drag it onto `RPI-RP2`. Boots straight into DOOM, no SD
     card and no loader required.
  2. **From the menu** — put it in the root of an SD card and pick it from the
     [rp2040-retro-loader](https://github.com/pondahai/rp2040-retro-loader) menu.

  ⚠️ Do **not** combine it with the `picotool load -o ...` commands above; the
  addresses don't match (its firmware is at `0x10004000`, map data at
  `0x10046000`). It is self-contained.

  **As of 2026-08-16 that file is louder**: the default sound and music volumes
  went from 8 to 15 (about +5.5dB). DOOM ships at half volume by default, and
  this firmware has nowhere to save settings — whatever you pick in the menu is
  back to the default on the next boot — so the compile-time initial value is
  the only thing that matters. The firmware files in this folder do **not**
  include the change and are still at the original volume.

  It lives in Releases rather than in version control because it's 4.2MB and
  would bloat the repo on every rebuild.

  ### File integrity (verified 2026-08-25)

  There was once a worry about drag-and-drop flashing failing. The usual cause
  is a bundle made by concatenating two UF2 files: each keeps its own `blockNo`
  0..N-1 and its own `numBlocks`, and the RP2040's USB mass-storage end uses
  exactly those fields to know when the transfer is complete — so it reboots
  early when the second file restarts at 0, leaving the firmware flashed and the
  map data missing.

  The file in Releases is **not a concatenation — it is a single regenerated
  UF2**. Taken apart and measured:

  - 8153 blocks, `blockNo` running 0..8152 with no breaks, `numBlocks` = 8153 on
    every block
  - one family ID throughout, `0xE48BFF56` (RP2040), consistent flags
  - addresses `0x10000000`–`0x101FD900` are **fully contiguous, no gaps**
  - padding is present: the 7096 bytes between the end of the firmware
    (`0x10044448`) and the start of the WAD (`0x10046000`) are filled with
    `0xFF`/`0x00`, likewise between the trampoline and `0x10004000`

  Layout: trampoline at `0x10000000` (boot2 + vector table), DOOM firmware at
  `0x10004000`, map data at `0x10046000`. That 16KB trampoline is why the
  standalone drag boots at all — the firmware is linked at `0x10004000`, but the
  bootrom still finds a valid image at `0x10000000` and jumps in.

  ---

  ## Old two-step install (not recommended)

  Installation method

  First, set the game console to USB upload mode, and then drag the corresponding .uf2 to the game console based on your
  version:

   1. Standard Version: doom_tiny.uf2
   2. Sound Fix Version: doom_tiny_ILI9341_sound_fix.uf2 (the better of the split builds, includes 2x oversampling audio, brightness boost, and
      voltage stability).

  Use instructions to upload map files

  Note! The map file offset depends on which firmware version you use:

   - For Standard Version:
    `picotool load -v -t bin ../doom1.whx -o 0x10040000`

   - For Sound Fix Version:
    `picotool load -v -t bin ../doom1.whx -o 0x10042000` (Important: Sound fix firmware is larger, offset must be shifted to
  avoid overlap).

  You can play after re-starting.
