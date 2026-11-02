module doom (
    input  wire        CLOCK_50,
    input  wire [0:0]  KEY,
    output wire [7:0]  LEDR,

    output wire [12:0] DRAM_ADDR,
    output wire [1:0]  DRAM_BA,
    output wire        DRAM_CAS_N,
    output wire        DRAM_CKE,
    output wire        DRAM_CLK,
    output wire        DRAM_CS_N,
    inout  wire [31:0] DRAM_DQ,
    output wire [3:0]  DRAM_DQM,
    output wire        DRAM_RAS_N,
    output wire        DRAM_WE_N,
	 
	 inout  wire        PS2_CLK,
	 inout  wire        PS2_DAT,

    output wire        VGA_CLK,
    output wire        VGA_HS,
    output wire        VGA_VS,
    output wire        VGA_BLANK_N,
    output wire        VGA_SYNC_N,
    output wire [7:0]  VGA_R,
    output wire [7:0]  VGA_G,
    output wire [7:0]  VGA_B,
	 
	 input  wire        AUD_BCLK,
	 input  wire        AUD_DACLRCK,
 	 output wire        AUD_DACDAT,
	 output wire        AUD_XCK,

	 output wire        I2C_SCLK,
	 inout  wire        I2C_SDAT
	 
);

    wire pll_locked;
    wire sdram_reset_n;

    assign sdram_reset_n = KEY[0] & pll_locked;

    doom_system u_system (
        .clk_clk                 (CLOCK_50),
        .reset_reset_n           (KEY[0]),
        .leds_export             (LEDR),

        .sdram_pll_reset_export  (~KEY[0]),
        .sdram_pll_locked_export (pll_locked),
        .sdram_reset_reset_n     (sdram_reset_n),
        .sdram_clk_clk           (DRAM_CLK),

        .sdram_zs_addr           (DRAM_ADDR),
        .sdram_zs_ba             (DRAM_BA),
        .sdram_zs_cas_n          (DRAM_CAS_N),
        .sdram_zs_cke            (DRAM_CKE),
        .sdram_zs_cs_n           (DRAM_CS_N),
        .sdram_zs_dq             (DRAM_DQ),
        .sdram_zs_dqm            (DRAM_DQM),
        .sdram_zs_ras_n          (DRAM_RAS_N),
        .sdram_zs_we_n           (DRAM_WE_N),
		  
		  .ps2_CLK (PS2_CLK),
		  .ps2_DAT (PS2_DAT),
		  
        .video_reset_reset_n (sdram_reset_n),

        .vga_CLK            (VGA_CLK),
        .vga_HS             (VGA_HS),
        .vga_VS             (VGA_VS),
        .vga_BLANK          (VGA_BLANK_N),
        .vga_SYNC           (VGA_SYNC_N),
        .vga_R              (VGA_R),
        .vga_G              (VGA_G),
        .vga_B              (VGA_B),
		  
        .audio_clk_clk  (AUD_XCK),

        .audio_BCLK     (AUD_BCLK),
        .audio_DACDAT   (AUD_DACDAT),
        .audio_DACLRCK  (AUD_DACLRCK),

        .av_config_SCLK (I2C_SCLK),
        .av_config_SDAT (I2C_SDAT)
		  
    );

endmodule