module doom (
    input  wire        CLOCK_50,
    input  wire [0:0]  KEY,

    input  wire        PS2_CLK,
    input  wire        PS2_DAT,

    output wire [7:0]  VGA_R,
    output wire [7:0]  VGA_G,
    output wire [7:0]  VGA_B,
    output wire        VGA_CLK,
    output wire        VGA_HS,
    output wire        VGA_VS,
    output wire        VGA_BLANK_N,
    output wire        VGA_SYNC_N,

    output wire [12:0] DRAM_ADDR,
    output wire [1:0]  DRAM_BA,
    output wire        DRAM_CAS_N,
    output wire        DRAM_RAS_N,
    output wire        DRAM_WE_N,
    output wire        DRAM_CS_N,
    output wire        DRAM_CKE,
    output wire        DRAM_CLK,
    output wire [3:0]  DRAM_DQM,
    inout  wire [31:0] DRAM_DQ
);

    wire debug_reset;
    wire system_reset_n;

    reset_control reset_inst (
        .clk            (CLOCK_50),
        .reset_button_n (KEY[0]),
        .debug_reset    (debug_reset),
        .system_reset_n (system_reset_n)
    );

    // Required by the reused custom SDRAM controller.
    assign DRAM_CLK = ~CLOCK_50;

    doom_system system_inst (
        .clk_clk           (CLOCK_50),
        .reset_reset_n     (system_reset_n),
        .debug_reset_reset (debug_reset),

        .io_ps2_clk        (PS2_CLK),
        .io_ps2_dat        (PS2_DAT),

        .io_vga_r          (VGA_R),
        .io_vga_g          (VGA_G),
        .io_vga_b          (VGA_B),
        .io_vga_clk        (VGA_CLK),
        .io_vga_hs         (VGA_HS),
        .io_vga_vs         (VGA_VS),
        .io_vga_blank_n    (VGA_BLANK_N),
        .io_vga_sync_n     (VGA_SYNC_N),

        .dram_addr         (DRAM_ADDR),
        .dram_ba           (DRAM_BA),
        .dram_cas_n        (DRAM_CAS_N),
        .dram_ras_n        (DRAM_RAS_N),
        .dram_we_n         (DRAM_WE_N),
        .dram_cs_n         (DRAM_CS_N),
        .dram_cke          (DRAM_CKE),
        .dram_dqm          (DRAM_DQM),
        .dram_dq           (DRAM_DQ),
        .dram_ready        ()
    );

endmodule