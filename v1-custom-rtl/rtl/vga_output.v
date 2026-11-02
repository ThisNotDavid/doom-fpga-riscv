module vga_output (
    input  wire        clk,
    input  wire        reset,
    input  wire        phase,

    input  wire        hsync_raw,
    input  wire        vsync_raw,
    input  wire        image_active,
    input  wire        display_enabled,
    input  wire [23:0] rgb,

    output wire [7:0]  vga_r,
    output wire [7:0]  vga_g,
    output wire [7:0]  vga_b,
    output reg         vga_clk,
    output wire        vga_hs,
    output wire        vga_vs,
    output wire        vga_blank_n,
    output wire        vga_sync_n
);

    reg hs1;
    reg hs2;
    reg hs3;

    reg vs1;
    reg vs2;
    reg vs3;

    reg active1;
    reg active2;

    // Preserve the reference's output alignment.
    always @(posedge clk) begin
        if (reset) begin
            hs1     <= 1'b1;
            hs2     <= 1'b1;
            hs3     <= 1'b1;

            vs1     <= 1'b1;
            vs2     <= 1'b1;
            vs3     <= 1'b1;

            active1 <= 1'b0;
            active2 <= 1'b0;
        end else begin
            hs1 <= hsync_raw;
            hs2 <= hs1;
            hs3 <= hs2;

            vs1 <= vsync_raw;
            vs2 <= vs1;
            vs3 <= vs2;

            active1 <= image_active && display_enabled;
            active2 <= active1;
        end
    end

    // Forward the pixel clock to the external VGA DAC.
    // Internal video logic continues to use the 50 MHz clock.
    always @(negedge clk) begin
        if (reset)
            vga_clk <= 1'b0;
        else
            vga_clk <= phase;
    end

    assign vga_r = active2 ? rgb[23:16] : 8'd0;
    assign vga_g = active2 ? rgb[15:8]  : 8'd0;
    assign vga_b = active2 ? rgb[7:0]   : 8'd0;

    assign vga_hs      = hs3;
    assign vga_vs      = vs3;
    assign vga_blank_n = active2;
    assign vga_sync_n  = 1'b0;

endmodule