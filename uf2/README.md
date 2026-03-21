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
