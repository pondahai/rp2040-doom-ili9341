# RP2040-Doom 客製化修改備份 (user_mods_v2)

改編自 https://github.com/kilograham/rp2040-doom

感謝kilograham做了絕大部分的貢獻
他把doom源碼做了很大的改寫
讓doom可以在rp2040(樹莓派pico)上運行
我只是修改成可以在LCD(ILI9341)上顯示

我在這裡只有放我改過的源碼
並且在修改的地方插入//dahai這樣的記號
主要的修改邏輯是
1.關掉用不到或是衝突的GPIO
2.將原本的scanvideo輸出關閉，改成將緩衝區資料送到LCD

Adapted from https://github.com/kilograham/rp2040-doom
Thanks to kilograham for making most of his contributions.
He made a big rewrite of the doom source code.
Let doom run on rp2040 (Raspberry Pi pico)

I just modified it so that it can be displayed on the LCD (ILI9341).
I only put the source code I changed here.
And insert a mark like //dahai in the modified place.
The main modification logic is

1. Turn off the useless or conflicting GPIO
2. Turn off the original scanvideo output and send the buffer data to the LCD instead.

---

本資料夾彙整了相對於 GitHub 原作者 (`kilograham/rp2040-doom` `origin/rp2` 分支) 的所有經過深度客製化、修改及新增的原始碼檔案。本備份可確保在未來更新或更換環境時，不會遺失您針對個人硬體量身打造的驅動與設定。

---

## 📂 檔案樹狀結構

只有與原版不相同的檔案才會被複製到這份精簡備份集中：

```text
user_mods_v2\
└── software\
    ├── CMakeLists.txt             (外層專案設定)
    ├── prebuilt_tools.cmake       (新增的編譯輔具)
    └── src\
        ├── CMakeLists.txt
        ├── i_main.c
        ├── i_video.h
        ├── m_controls.c
        └── pico\
            ├── CMakeLists.txt
            ├── i_input.c
            ├── i_picosound.c
            ├── i_video.c
            ├── magc.h             (新增的硬體暫存器設定)
            ├── piconet.c
            └── video_doom.pio
```

---

## 📝 源碼比對表與差異簡介

以下表格列出本備份集相對於原作者版本到底改了什麼：

| 檔案位置 (user_mods_v2 內) | 對應的 Github 原版檔案 | 差異描述 |
| :--- | :--- | :--- |
| `software/CMakeLists.txt` | `CMakeLists.txt` (根目錄) | 將 CMake 最低版本要求改為 3.12，並引入 `prebuilt_tools.cmake`。 |
| `software/prebuilt_tools.cmake` | **(無 / 新增檔案)** | 指定 `pioasm` 與 `elf2uf2` 在 Windows 本機上的絕對路徑，以修復編譯環境問題。 |
| `software/src/CMakeLists.txt` | `src/CMakeLists.txt` | 更改 I2C 腳位由預設 18/19 改為 **14/15**，關閉 UART 輸出，註解停用 USB，並修改 WAD 記憶體位址。 |
| `software/src/i_main.c` | `src/i_main.c` | 移除預設 RP2350 QMI 初始化邏輯。新增開機 `sleep_ms(500)`，設定電壓 1.30V。 |
| `software/src/i_video.h` | `src/i_video.h` | 增加 `#ifdef ILI9341` 與 `#ifdef ST7789` 支援，固定這兩種螢幕的系統解析度為 320x200。 |
| `software/src/m_controls.c` | `src/m_controls.c` | 將換武器按鍵 `key_nextweapon` 綁定改為 `'+'`。 |
| `software/src/pico/CMakeLists.txt` | `src/pico/CMakeLists.txt` | 將新增的自製檔案（如 `magc.h`）與客製化 LCD_CONTROLLER 的變數參數納入編譯。 |
| `software/src/pico/i_input.c` | `src/pico/i_input.c` | 重寫並修改按鍵與 GPIO 的對應讀取邏輯，以配合您專屬自製的擴充搖桿硬體。 |
| `software/src/pico/i_picosound.c` | `src/pico/i_picosound.c` | **大規模翻修（約 400 行）**。實作了 2x Oversampling、8-bit PWM 以及進階的音訊輸出邏輯。 |
| `software/src/pico/i_video.c` | `src/pico/i_video.c` | 增加硬體 SPI LCD 初始化函式 (`ili9341_...`, `st7789_...`)，並在主流程中宣告與控制 `LED_PIN` 狀態。 |
| `software/src/pico/magc.h` | **(無 / 新增檔案)** | 統一集中管理多種螢幕（ILI9341/ST7789）的 DCS 硬體驅動暫存器參數、解析度，以及自定義的手把按鍵代碼對照表。 |
| `software/src/pico/piconet.c` | `src/pico/piconet.c` | 變更硬體定時器中斷初始化的寫法，取代原有的 `TIMER_ALARM_IRQ_NUM` 配置。 |
| `software/src/pico/video_doom.pio` | `src/pico/video_doom.pio` | 將原有實體層的 PIO 掃描線推掃控制指令（如 `out`, `wait irq`, `jmp` 等）予以註解。 |

---

## 🚀 源碼修改核心摘要

這份客製化備份檔主要統整了針對本 RP2040 Doom 專案的四大系統特化修改：

1. **視覺介面硬體替換 (Video / LCD)**：
   我們不沿用作者的預設螢幕驅動，而是透過新增的 `magc.h` 與修改的 `i_video.c`，成功驅動了 **ILI9341** 與 **ST7789** 面板，並處理了畫面解析度和初始化 DCS 位址空間。

2. **音訊輸出系統大幅升級 (Audio / PWM)**：
   在 `i_picosound.c` 進行了達四百行的深層改寫。放棄原生普通的音效輸出方式，升級支援了 **2x Oversampling (兩倍超取樣)** 及 **8-bit PWM 輸出優化**，以獲得更無雜訊及高清晰的遊戲音源。

3. **專用的硬體控制對應 (Controls / Input)**：
   透過修改 `i_input.c` 與 `m_controls.c`，配合 `magc.h` 定義的新按鈕腳位與按鍵映射（如武器切換、方向鍵），完美連結了您的自製實體硬體版。

4. **系統穩定性與編譯環境修正 (Core / System)**：
   在開機前匯注等待緩衝 (`sleep_ms(500)`) 來提高冷開機穩定性，正確修正 I2C 腳位（14 與 15）。同時加入 `prebuilt_tools.cmake` 處理了 Windows 環境獨有的 pico-sdk 編譯器路徑遺失問題，確保專案隨時可一件順利編譯打包出 `.uf2` 檔。
