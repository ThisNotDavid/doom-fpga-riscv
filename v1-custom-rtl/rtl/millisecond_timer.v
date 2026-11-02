module millisecond_timer (
    input  wire        clk,
    input  wire        reset,
    output reg  [31:0] milliseconds
);

    reg [15:0] divider;

    // 50000 cycles at 50 MHz = 1 millisecond.
    always @(posedge clk) begin
        if (reset) begin
            divider      <= 16'd0;
            milliseconds <= 32'd0;
        end else begin
            if (divider == 16'd49999) begin
                divider      <= 16'd0;
                milliseconds <= milliseconds + 32'd1;
            end else begin
                divider <= divider + 16'd1;
            end
        end
    end

endmodule