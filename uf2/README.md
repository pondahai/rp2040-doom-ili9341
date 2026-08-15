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

  ## DOOM.uf2 — 給 rp2040-retro-loader 用的版本

  `DOOM.uf2` 跟上面那些不一樣：**韌體與地圖檔已經包在同一份檔案裡**，
  不需要再用 `picotool` 另外燒 `doom1.whx`。

  這一份是給 [rp2040-retro-loader](https://github.com/pondahai/rp2040-retro-loader)
  用的，韌體連結到 `0x10004000`（前 16KB 留給載入器或跳板），地圖檔在
  `0x10046000`。兩種用法：

  1. **獨立燒錄**：直接拖進 `RPI-RP2`。檔案最前面帶了跳板，開機就進 DOOM，
     不需要 SD 卡
  2. **從選單載入**：跟 `DOOM.RAW` 一起放到 SD 卡**根目錄**，開機由載入器
     的圖形選單挑

  `DOOM.RAW` 是選單用的封面圖（96×96 RGB565 big-endian，固定 18432 bytes），
  檔名必須跟 `.uf2` 一致才會被認出來。沒有封面也能選，只是顯示成灰色色塊。

  ⚠️ 從選單選這一項要等 **30–45 秒**（要寫入約 2MB），畫面看起來像當機是正常的，
  不要拔電。

  ⚠️ 這份韌體**不能**用上面那些 `picotool load -o 0x10042000` 的指令搭配，
  位址對不上。它自己就是完整的。

  ---

  ## DOOM.uf2 — for rp2040-retro-loader

  Unlike the files above, `DOOM.uf2` **bundles the firmware and the map data in
  a single file** — there is no separate `picotool` step for `doom1.whx`.

  It is built for [rp2040-retro-loader](https://github.com/pondahai/rp2040-retro-loader):
  the firmware is linked at `0x10004000` (the first 16KB is reserved for the
  loader or the trampoline) and the map data sits at `0x10046000`. Two ways to
  use it:

  1. **Standalone**: drag it onto `RPI-RP2`. A trampoline is prepended, so it
     boots straight into DOOM with no SD card involved.
  2. **From the menu**: put it in the **root** of the SD card together with
     `DOOM.RAW` and pick it from the loader's graphical menu.

  `DOOM.RAW` is the menu cover art (96×96 RGB565 big-endian, exactly 18432
  bytes). Its name must match the `.uf2` or it will not be picked up; entries
  without cover art still work and show a grey block instead.

  ⚠️ Selecting it from the menu takes **30–45 seconds** (about 2MB gets written).
  The screen looks frozen while this happens — do not pull the power.

  ⚠️ Do **not** combine this firmware with the `picotool load -o 0x10042000`
  commands above; the addresses do not match. This file is self-contained.

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
