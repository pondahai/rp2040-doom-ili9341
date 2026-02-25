add_executable(Pioasm IMPORTED)
set_property(TARGET Pioasm PROPERTY IMPORTED_LOCATION "C:/Users/Dell/.pico-sdk/tools/2.2.0/pioasm/pioasm.exe")
set(Pioasm_FOUND 1 CACHE INTERNAL "")

add_executable(ELF2UF2 IMPORTED)
set_property(TARGET ELF2UF2 PROPERTY IMPORTED_LOCATION "C:/Users/Dell/.arduino-create/arduino/rp2040tools/1.0.6/elf2uf2.exe")
set(ELF2UF2_FOUND 1 CACHE INTERNAL "")
