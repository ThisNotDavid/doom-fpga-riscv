module vga_top (
    input  wire        clk,
    input  wire        reset,

    // Decoded CPU requests from doom_io_bus.
    input  wire        fb_write,
    input  wire        palette_write,
    input  wire        submit,

    // CPU address is in 32-bit words, not bytes.
    input  wire [15:0] cpu_address,
    input  wire [31:0] cpu_writedata,
    input  wire [3:0]  cpu_byteenable,

    output wire [31:0] display_status,

    output wire [7:0]  vga_r,
    output wire [7:0]  vga_g,
    output wire [7:0]  vga_b,
    output wire        vga_clk,
    output wire        vga_hs,
    output wire        vga_vs,
    output wire        vga_blank_n,
    output wire        vga_sync_n
);

    wire        phase;
    wire [9:0]  x;
    wire [9:0]  y;
    wire        hsync_raw;
    wire        vsync_raw;
    wire        vblank_start;

    wire        image_active;
    wire [15:0] pixel_address;
    wire [7:0]  pixel_index;
    wire [23:0] rgb;

    wire        front_bank;
    wire        pending;
    wire        display_enabled;

    wire        framebuffer_write_allowed;
    wire        palette_write_allowed;
    wire        submit_request;

    // Writes may only target the back bank.
    // Freeze both banks while a submission is pending.
    assign framebuffer_write_allowed =
        fb_write &&
        !reset &&
        !pending &&
        (cpu_address[14] != front_bank);

    assign palette_write_allowed =
        palette_write &&
        !reset &&
        !pending &&
        (cpu_address[8] != front_bank);

    // Submit uses bits 1:0 of the low byte.
    assign submit_request =
        submit && cpu_byteenable[0] && !reset;

    // Preserve the reference's status-register layout.
    assign display_status = {
        29'd0,
        display_enabled,
        pending,
        front_bank
    };

    vga_timing timing (
        .clk          (clk),
        .reset        (reset),
        .phase        (phase),
        .x            (x),
        .y            (y),
        .hsync_raw    (hsync_raw),
        .vsync_raw    (vsync_raw),
        .vblank_start (vblank_start)
    );

    framebuffer_address address_generator (
        .x             (x),
        .y             (y),
        .image_active  (image_active),
        .pixel_address (pixel_address)
    );

    frame_swap_control frame_control (
        .clk              (clk),
        .reset            (reset),
        .vblank_start     (vblank_start),
        .submit           (submit_request),
        .submit_bank      (cpu_writedata[0]),
        .submit_enable    (cpu_writedata[1]),
        .front_bank       (front_bank),
        .pending          (pending),
        .display_enabled  (display_enabled)
    );

    framebuffer_memory framebuffers (
        .clk                (clk),
        .cpu_write          (framebuffer_write_allowed),
        .cpu_word_address   (cpu_address[14:0]),
        .cpu_writedata      (cpu_writedata),
        .cpu_byteenable     (cpu_byteenable),
        .front_bank         (front_bank),
        .scan_pixel_address (pixel_address),
        .pixel_index        (pixel_index)
    );

    palette_memory palette (
        .clk                 (clk),
        .cpu_write           (palette_write_allowed),
        .cpu_palette_address (cpu_address[8:0]),
        .cpu_writedata       (cpu_writedata[23:0]),
        .front_bank          (front_bank),
        .pixel_index         (pixel_index),
        .rgb                 (rgb)
    );

    vga_output output_stage (
        .clk             (clk),
        .reset           (reset),
        .phase           (phase),
        .hsync_raw       (hsync_raw),
        .vsync_raw       (vsync_raw),
        .image_active    (image_active),
        .display_enabled (display_enabled),
        .rgb             (rgb),
        .vga_r           (vga_r),
        .vga_g           (vga_g),
        .vga_b           (vga_b),
        .vga_clk         (vga_clk),
        .vga_hs          (vga_hs),
        .vga_vs          (vga_vs),
        .vga_blank_n     (vga_blank_n),
        .vga_sync_n      (vga_sync_n)
    );

endmodule