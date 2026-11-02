.section .rodata.wad,"a",@progbits
.balign 4

.global embedded_wad
embedded_wad:
.incbin "DOOM.WAD"  /* found via -Wa,-I (see CMakeLists.txt) */
embedded_wad_end:

.balign 4
.global embedded_wad_size
embedded_wad_size:
.word embedded_wad_end - embedded_wad

.global embedded_wad_name
embedded_wad_name:
.asciz "DOOM.WAD"
