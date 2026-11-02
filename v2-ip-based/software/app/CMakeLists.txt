cmake_minimum_required(VERSION 3.14)

add_subdirectory(../bsp bsp)
include(../bsp/toolchain.cmake)

project(doom)
enable_language(ASM)
enable_language(C)
enable_language(CXX)

add_executable(doom.elf
    src/doomgeneric_niosv.c
    src/ps2_keys.c
    src/ramfs.c
    src/niosv_fs.c
    src/wad.S
    src/doom_audio.c
    src/doom_diag.c
)

include("${CMAKE_CURRENT_SOURCE_DIR}/doom_sources.cmake")

set_source_files_properties(src/wad.S PROPERTIES
    COMPILE_OPTIONS "-Wa,-I${CMAKE_CURRENT_SOURCE_DIR}/../../assets"
    OBJECT_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/DOOM.WAD"
)

target_include_directories(doom.elf PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/doomgeneric"
)

target_compile_definitions(doom.elf PRIVATE
    CMAP256
    DOOMGENERIC_RESX=320
    DOOMGENERIC_RESY=200
    NORMALUNIX
    LINUX
    SNDSERV
    _DEFAULT_SOURCE
    FEATURE_SOUND
    DOOM_NIOSV
)

set_target_properties(doom.elf PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)

target_compile_options(doom.elf PRIVATE
    -O2
    -g
    "$<$<COMPILE_LANGUAGE:C>:-fno-strict-aliasing;-fwrapv;-ffunction-sections;-fdata-sections>"
)

target_link_options(doom.elf PRIVATE
    "-Wl,--gc-sections"
    "-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/doom.map"
)

target_link_libraries(doom.elf PRIVATE
    -T "${BspLinkerScript}" -nostdlib
    "${ExtraArchiveLibraries}"
    -Wl,--start-group
    "${BspLibraryName}" -lc -lstdc++ -lgcc -lm
    -Wl,--end-group
)

add_executable(audio_test.elf
    src/audio_test.c
)

set_target_properties(audio_test.elf PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)

target_compile_options(audio_test.elf PRIVATE -O2 -g)

target_link_libraries(audio_test.elf PRIVATE
    -T "${BspLinkerScript}" -nostdlib
    "${ExtraArchiveLibraries}"
    -Wl,--start-group
    "${BspLibraryName}" -lc -lstdc++ -lgcc -lm
    -Wl,--end-group
)

