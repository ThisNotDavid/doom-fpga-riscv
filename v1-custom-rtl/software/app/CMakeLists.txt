cmake_minimum_required(VERSION 3.14)

add_subdirectory(../bsp bsp)

include(../bsp/toolchain.cmake)

project(doom)

enable_language(ASM)
enable_language(C)
enable_language(CXX)

add_executable(doom.elf)

target_sources(doom.elf
    PRIVATE
        src/doomgeneric_niosv.c
        src/ps2_keys.c
        src/ramfs.c
        src/niosv_fs.c
        src/wad.S
)

include("${CMAKE_CURRENT_SOURCE_DIR}/doom_sources.cmake")

set_source_files_properties(src/wad.S PROPERTIES
    COMPILE_OPTIONS "-Wa,-I${CMAKE_CURRENT_SOURCE_DIR}/assets"
    OBJECT_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/DOOM.WAD"
)

target_include_directories(doom.elf
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/doomgeneric"
)

target_compile_definitions(doom.elf
    PRIVATE
        DOOM_NIOSV
        DOOM_FPS_OVERLAY
        CMAP256
        DOOMGENERIC_RESX=320
        DOOMGENERIC_RESY=200
        NORMALUNIX
        LINUX
        SNDSERV
        _DEFAULT_SOURCE
)

set_target_properties(doom.elf PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)

target_compile_options(doom.elf
    PRIVATE
        -O2
        -g
        "$<$<COMPILE_LANGUAGE:C>:-fno-strict-aliasing;-fwrapv;-ffunction-sections;-fdata-sections>"
)

target_link_options(doom.elf
    PRIVATE
        "-Wl,--gc-sections"
        "-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/doom.map"
)

target_link_libraries(doom.elf
    PRIVATE
        -T "${BspLinkerScript}" -nostdlib
        "${ExtraArchiveLibraries}"
        -Wl,--start-group "${BspLibraryName}" -lc -lstdc++ -lgcc -lm -Wl,--end-group
)

# Create objdump from ELF.
set(objdump doom.elf.objdump)
add_custom_command(
    OUTPUT "${objdump}"
    DEPENDS doom.elf
    COMMAND "${ToolchainObjdump}" "${ToolchainObjdumpFlags}" doom.elf >
            "${objdump}"
    COMMENT "Creating ${objdump}."
    VERBATIM
)
add_custom_target(create-objdump ALL DEPENDS "${objdump}")

# Report space free for stack + heap. Note that the file below is never created
# so the report is always output on build.
set(stack_report_file doom.elf.stack_report)
add_custom_command(
    OUTPUT "${stack_report_file}"
    DEPENDS doom.elf
    COMMAND niosv-stack-report -p "${ToolchainPrefix}" doom.elf
    COMMENT "Reporting memory available for stack + heap in doom.elf."
    VERBATIM
)
add_custom_target(niosv-stack-report ALL DEPENDS "${stack_report_file}")

# Generate HEX file(s) from doom.elf using elf2hex tool.
# Note : If ECC Full is enabled, width of 39 is set for NiosV TCM. Otherwise, 32.
add_custom_command(
    OUTPUT "BOOT_RAM.hex"
    DEPENDS doom.elf
    COMMAND elf2hex doom.elf -o BOOT_RAM.hex -b 0x00000000 -w 32 -e 0x00003FFF -r 4
    COMMENT "Creating BOOT_RAM.hex."
    VERBATIM
)
add_custom_command(
    OUTPUT "SDRAM.hex"
    DEPENDS doom.elf
    COMMAND elf2hex doom.elf -o SDRAM.hex -b 0x08000000 -w 32 -e 0x0FFFFFFF -r 4
    COMMENT "Creating SDRAM.hex."
    VERBATIM
)
add_custom_target(create-hex DEPENDS "BOOT_RAM.hex" "SDRAM.hex")
