module doom_io (
    input  wire        clk,
    input  wire        reset,

    // Avalon-MM interface.
    input  wire [15:0] address,
    input  wire        read,
    input  wire        write,
    input  wire [31:0] writedata,
    input  wire [3:0]  byteenable,
    output wire [31:0] readdata,

    // PS/2 board pins.
    input  wire        ps2_clk,
    input  wire        ps2_dat,

    // VGA board pins.
    output wire [7:0]  vga_r,
    output wire [7:0]  vga_g,
    output wire [7:0]  vga_b,
    output wire        vga_clk,
    output wire        vga_hs,
    output wire        vga_vs,
    output wire        vga_blank_n,
    output wire        vga_sync_n
);

    wire        fb_write;
    wire        palette_write;
    wire        submit;

    wire        key_pop;
    wire [1:0]  key_clear_errors;

    wire [31:0] display_status;
    wire [31:0] milliseconds;
    wire [31:0] key_data;
    wire [31:0] key_errors;

    doom_io_bus bus_interface (
        .clk              (clk),
        .reset            (reset),

        .address          (address),
        .read             (read),
        .write            (write),
        .writedata        (writedata),
        .byteenable       (byteenable),
        .readdata         (readdata),

        .display_status   (display_status),
        .milliseconds     (milliseconds),
        .key_data         (key_data),
        .key_errors       (key_errors),

        .fb_write         (fb_write),
        .palette_write    (palette_write),
        .submit           (submit),
        .key_pop          (key_pop),
        .key_clear_errors (key_clear_errors)
    );

    millisecond_timer timer (
        .clk          (clk),
        .reset        (reset),
        .milliseconds (milliseconds)
    );

    keyboard_top keyboard (
        .clk          (clk),
        .reset        (reset),

        .ps2_clk      (ps2_clk),
        .ps2_dat      (ps2_dat),

        .pop          (key_pop),
        .clear_errors (key_clear_errors),

        .key_data     (key_data),
        .key_errors   (key_errors)
    );

    vga_top video (
        .clk            (clk),
        .reset          (reset),

        .fb_write       (fb_write),
        .palette_write  (palette_write),
        .submit         (submit),

        .cpu_address    (address),
        .cpu_writedata  (writedata),
        .cpu_byteenable (byteenable),

        .display_status (display_status),

        .vga_r          (vga_r),
        .vga_g          (vga_g),
        .vga_b          (vga_b),
        .vga_clk        (vga_clk),
        .vga_hs         (vga_hs),
        .vga_vs         (vga_vs),
        .vga_blank_n    (vga_blank_n),
        .vga_sync_n     (vga_sync_n)
    );

endmodule