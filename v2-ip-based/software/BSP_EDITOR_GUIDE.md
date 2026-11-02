# BSP Editor checklist — DE2-115 DOOM

## Purpose

Regenerate the Nios V BSP after hardware changes, including the VGA pipeline. Keep diagnostic code, heap, and stack in on-chip RAM while testing SDRAM and video.

This is a GUI checklist. No Tcl or Python script is required.

## Current build issue

The diagnostic build failed because `PIXEL_DMA_BASE` was undefined. The inspected `software/bsp/system.h` was generated before the VGA hardware was added and contained no pixel-DMA definitions. Rebuilding the application alone does not refresh the BSP hardware description.

## 1. Open the BSP

1. Save the Platform Designer system and generate its HDL after any hardware changes.
2. Close and reopen Nios V BSP Editor.
3. Choose **File > Open** and select:

   `C:\FPGA\vecad-fpga-doom\software\bsp\settings.bsp`

4. Confirm the SOPC Information File points to:

   `C:\FPGA\vecad-fpga-doom\hardware\doom_system.sopcinfo`

5. Accept any prompt to update settings for changed hardware.
6. Confirm CPU is `cpu`, operating system is **Altera HAL**, and BSP version is **default**.

The BSP target directory must resolve to:

`C:\FPGA\vecad-fpga-doom\software\bsp`

## 2. Check HAL settings

Under **Main > Settings**, use:

| Setting | Value |
|---|---|
| `hal.sys_clk_timer` | `sys_timer` |
| `hal.timestamp_timer` | `none` |
| `hal.stdin` | `jtag_uart` |
| `hal.stdout` | `jtag_uart` |
| `hal.stderr` | `jtag_uart` |
| `hal.enable_c_plus_plus` | Disabled |
| `hal.enable_small_c_library` | Enabled, if available |
| `hal.enable_reduced_device_drivers` | Disabled |
| `hal.enable_lightweight_device_driver_api` | Disabled |
| `hal.enable_sim_optimize` | Disabled |

Keep the existing working startup/linker options. For the current JTAG-loaded diagnostic, `allow_code_at_reset` remains enabled.

## 3. Check linker placement

Open **Linker Script**. Confirm the hardware memory map contains:

| Memory | Hardware base | Hardware span |
|---|---|---|
| `onchip_ram` | `0x00000000` | 65,536 bytes |
| `sdram` | `0x20000000` | 134,217,728 bytes |

The generated linker may reserve the first 32 bytes for reset and show the remaining on-chip region starting at `0x20`. That is expected.

Map all these sections to **onchip_ram**:

- `.text`
- `.exceptions`
- `.rodata`
- `.rwdata`
- `.bss`
- `.heap`
- `.stack`

Do not place the diagnostic program in SDRAM. An empty automatically generated `.sdram` section is not itself a problem; do not explicitly place diagnostic objects there.

## 4. Generate and verify

1. Click **Generate**.
2. Check the **Problems** tab for errors.
3. Save the settings.
4. Open the generated file:

   `C:\FPGA\vecad-fpga-doom\software\bsp\system.h`

5. Confirm the header now describes the VGA-era hardware. Search for `PIXEL`, `SDRAM`, and `RGB`.

Expected hardware addresses:

| Component | Base |
|---|---|
| LED PIO | `0x10002000` |
| SDRAM PLL registers | `0x10003000` |
| Pixel DMA control registers | `0x10004000` |
| RGB resampler control registers | `0x10005000` |
| SDRAM | `0x20000000` |

The application currently expects `PIXEL_DMA_BASE`. Confirm this macro exists and equals `0x10004000`. If the regenerated header uses a different interface-qualified name, use that actual generated macro in the application. Do not manually edit `system.h` or add a guessed address merely to hide a stale BSP.

Confirm `SDRAM_SPAN` is 134,217,728 bytes (`0x08000000`), the HAL system clock uses `sys_timer`, and the timestamp timer is `none`.

## 5. If reopening does not update the hardware

Use **File > New BSP** with:

- SOPC Information File: the latest `hardware\doom_system.sopcinfo`.
- CPU: `cpu`.
- Operating system: **Altera HAL**.
- Version: **default**.
- BSP target directory: `C:\FPGA\vecad-fpga-doom\software\bsp`.
- Settings file: `settings.bsp` in that directory.

This recreates settings in the existing BSP destination. Reapply the HAL and linker settings above before generating. Do not delete the application folder.

## 6. Rebuild in RiscFree

1. Refresh the BSP and application projects with **F5**.
2. If `main.c` was moved into `src`, ensure the application's `target_sources` lists `src/main.c`.
3. Clean and build the BSP and application.
4. Confirm the build succeeds and the ELF modification time updates.
5. Use the new ELF with the matching VGA-enabled `.sof` on the board.

The current ELF location is:

`C:\FPGA\vecad-fpga-doom\software\app\build\Default\app.elf`

A failed build can leave an older ELF in that location. Do not mistake its presence for a successful new build.

## VGA diagnostic checkpoint

- Front framebuffer: `0x27F00000`.
- Back framebuffer: `0x27F40000`.
- Format: 320 × 240 RGB565, consecutive addressing.
- CPU Peripheral Region B: base `0x27F00000`, size 1 MiB, configured in hardware as uncached.
- This hardware cache setting is not configured by BSP Editor.
- Full SDRAM and byte/halfword tests previously passed.
- VGA currently shows corrupted pixels; the diagnostic application build must first be repaired.
- Once the diagnostic runs, capture messages A through F, DMA register values, framebuffer sample values, and the final LED state.
- External SDRAM and VGA I/O timing constraints remain outstanding; functional passes do not establish timing closure.
