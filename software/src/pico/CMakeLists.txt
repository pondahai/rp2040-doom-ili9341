add_library(common_pico INTERFACE)
target_sources(common_pico INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/i_glob.c
        ${CMAKE_CURRENT_LIST_DIR}/i_input.c
        ${CMAKE_CURRENT_LIST_DIR}/i_system.c
        ${CMAKE_CURRENT_LIST_DIR}/piconet.c
        ${CMAKE_CURRENT_LIST_DIR}/i_timer.c
        ${CMAKE_CURRENT_LIST_DIR}/i_video.c
        ${CMAKE_CURRENT_LIST_DIR}/stubs.c

        ${CMAKE_CURRENT_LIST_DIR}/i_picosound.c
)
if (PICO_ON_DEVICE)
    target_sources(common_pico INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/blit.S
            ${CMAKE_CURRENT_LIST_DIR}/picoflash.c
            )
    pico_wrap_function(common_pico malloc)
    pico_wrap_function(common_pico calloc)
    pico_wrap_function(common_pico free)
    target_compile_definitions(common_pico INTERFACE
            PICO_HEAP_SIZE=0
            USE_ZONE_FOR_MALLOC=1
    )
    
    target_link_libraries(common_pico INTERFACE  hardware_i2c)
endif()
target_include_directories(common_pico INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/..
        ${CMAKE_CURRENT_LIST_DIR}/../../textscreen
        )

target_compile_definitions(common_pico INTERFACE
        NO_USE_MOUSE=1
        #dahai
        PICO_AUDIO_I2S_PIO=1
        PICO_AUDIO_I2S_DMA_IRQ=1
        )

pico_generate_pio_header(common_pico ${CMAKE_CURRENT_LIST_DIR}/video_doom.pio)
target_link_libraries(common_pico INTERFACE pico_stdlib pico_multicore pico_scanvideo_dpi)

add_library(pico_cd INTERFACE)
if (TARGET tinyusb_host)
    target_link_libraries(pico_cd INTERFACE tinyusb_host)
endif()

#
#
#
#dahai
# disable scanvideo's pin
target_compile_definitions(common_pico INTERFACE
    PICO_SCANVIDEO_COLOR_PIN_BASE=30 
    PICO_SCANVIDEO_SYNC_PIN_BASE=30
    PICO_AUDIO_I2S_DATA_PIN=30
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=30
)

target_link_libraries(common_pico INTERFACE hardware_spi)

set(SD_SPI "spi1" CACHE STRING "Specify the SPI bus for the SD card")
set(SD_CS "13" CACHE STRING "Specify the Chip Select GPIO pin for the SD card")
set(SD_SCK "10" CACHE STRING "Specify de Clock GPIO pin for the SD card")
set(SD_MOSI "11" CACHE STRING "Select the Master Out Slave In GPIO pin for the SD card")
set(SD_MISO "12" CACHE STRING "Select the Master In Slave Out GPIO pin for the SD card")

set(LCD_CONTROLLER "ILI9341" CACHE STRING "Select the LCD controller type")
#set(LCD_CONTROLLER "ST7789" CACHE STRING "Select the LCD controller type")

set(LCD_SPI "spi0")
set(LCD_DC "20")
set(LCD_CS "17")
set(LCD_CLK "18")
set(LCD_MOSI "19")
set(LCD_RST "21")
set(LCD_BL "22")
set(LCD_MISO "-1")

message("SD card SPI         : ${SD_SPI}")
message("SD card CS          : ${SD_CS}")
message("SD card SCK         : ${SD_SCK}")
message("SD card MOSI        : ${SD_MOSI}")
message("SD card MISO        : ${SD_MISO}")
message("LCD CONTROLLER      : ${LCD_CONTROLLER}")
message("LCD SPI             : ${LCD_SPI}")
message("LCD DC              : ${LCD_DC}")
message("LCD CS              : ${LCD_CS}")
message("LCD CLK             : ${LCD_CLK}")
message("LCD MOSI            : ${LCD_MOSI}")
message("LCD RST             : ${LCD_RST}")
message("LCD BL              : ${LCD_BL}")
message("LCD MISO            : ${LCD_MISO}")
target_compile_definitions(common_pico INTERFACE
    SDCARD_SPI_BUS=${SD_SPI}
    SDCARD_PIN_SPI0_CS=${SD_CS}
    SDCARD_PIN_SPI0_SCK=${SD_SCK}
    SDCARD_PIN_SPI0_MOSI=${SD_MOSI}
    SDCARD_PIN_SPI0_MISO=${SD_MISO}
    DISPLAY_SPI_PORT=${LCD_SPI}
    DISPLAY_PIN_DC=${LCD_DC}
    DISPLAY_PIN_CS=${LCD_CS}
    DISPLAY_PIN_CLK=${LCD_CLK}
    DISPLAY_PIN_MOSI=${LCD_MOSI}
    DISPLAY_PIN_RST=${LCD_RST}
    DISPLAY_PIN_BL=${LCD_BL}
    DISPLAY_PIN_MISO=${LCD_MISO}
)

if(LCD_CONTROLLER STREQUAL "ILI9341")
    add_compile_definitions(ILI9341)
    target_compile_definitions(common_pico INTERFACE
        ILI9341=1
        )
elseif(LCD_CONTROLLER STREQUAL "ST7789")
    add_compile_definitions(ST7789)
    target_compile_definitions(common_pico INTERFACE
        ST7789=1
        )
endif()

