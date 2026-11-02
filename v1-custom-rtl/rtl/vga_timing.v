module vga_timing (
    input  wire       clk,
    input  wire       reset,

    output reg        phase,
    output reg  [9:0] x,
    output reg  [9:0] y,

    output wire       hsync_raw,
    output wire       vsync_raw,
    output wire       vblank_start
);

    // Horizontal: 640 visible + 16 front + 96 sync + 48 back.
    // Vertical:   480 visible + 10 front + 2 sync + 33 back.
    assign hsync_raw = !((x >= 10'd656) && (x < 10'd752));
    assign vsync_raw = !((y >= 10'd490) && (y < 10'd492));

    // One system-clock-wide event at the first vertical blank line.
    assign vblank_start =
        phase && (x == 10'd0) && (y == 10'd480);

    always @(posedge clk) begin
        if (reset) begin
            phase <= 1'b0;
            x     <= 10'd0;
            y     <= 10'd0;
        end else begin
            phase <= ~phase;

            if (phase) begin
                if (x == 10'd799) begin
                    x <= 10'd0;

                    if (y == 10'd524)
                        y <= 10'd0;
                    else
                        y <= y + 10'd1;
                end else begin
                    x <= x + 10'd1;
                end
            end
        end
    end

endmodule