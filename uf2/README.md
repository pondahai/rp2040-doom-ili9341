  安裝方法

  首先將遊戲機設置為usb上傳模式，然後根據版本選擇對應的 .uf2 檔案拖拉至遊戲機：

   1. 標準版 (Standard): doom_tiny.uf2
   2. 音效修正版 (Sound Fix): doom_tiny_ILI9341_sound_fix.uf2 (推薦，包含 2x 超取樣音效、螢幕亮度提升與穩定性優化)

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

  因為檔案有 4.2MB,放 Releases 而不放進版控,免得 repo 每次重編都胖一圈。

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

  It lives in Releases rather than in version control because it's 4.2MB and
  would bloat the repo on every rebuild.

  ---

  Installation method

  First, set the game console to USB upload mode, and then drag the corresponding .uf2 to the game console based on your
  version:

   1. Standard Version: doom_tiny.uf2
   2. Sound Fix Version: doom_tiny_ILI9341_sound_fix.uf2 (Recommended, includes 2x oversampling audio, brightness boost, and
      voltage stability).

  Use instructions to upload map files

  Note! The map file offset depends on which firmware version you use:

   - For Standard Version:
    `picotool load -v -t bin ../doom1.whx -o 0x10040000`

   - For Sound Fix Version:
    `picotool load -v -t bin ../doom1.whx -o 0x10042000` (Important: Sound fix firmware is larger, offset must be shifted to
  avoid overlap).

  You can play after re-starting.
