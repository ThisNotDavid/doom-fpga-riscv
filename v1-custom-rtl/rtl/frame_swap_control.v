module frame_swap_control (
    input  wire clk,
    input  wire reset,

    input  wire vblank_start,

    input  wire submit,
    input  wire submit_bank,
    input  wire submit_enable,

    output reg  front_bank,
    output reg  pending,
    output reg  display_enabled
);

    reg requested_bank;
    reg requested_enable;

    always @(posedge clk) begin
        if (reset) begin
            front_bank       <= 1'b0;
            pending          <= 1'b0;
            display_enabled  <= 1'b0;
            requested_bank   <= 1'b0;
            requested_enable <= 1'b0;
        end else begin
            // Accept only one outstanding submission.
            if (submit && !pending) begin
                requested_bank   <= submit_bank;
                requested_enable <= submit_enable;
                pending          <= 1'b1;
            end

            // Commit a previously accepted submission.
            if (vblank_start && pending) begin
                front_bank      <= requested_bank;
                display_enabled <= requested_enable;
                pending         <= 1'b0;
            end
        end
    end

endmodule