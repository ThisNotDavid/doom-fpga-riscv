module reset_control (
    input  wire clk,
    input  wire reset_button_n,
    input  wire debug_reset,

    output wire system_reset_n
);

    // FPGA power-up initialization, supported by Quartus.
    reg [2:0] power_reset = 3'b111;

    always @(posedge clk or negedge reset_button_n) begin
        if (!reset_button_n)
            power_reset <= 3'b111;
        else
            power_reset <= {power_reset[1:0], 1'b0};
    end

    // Platform Designer exports an active-low reset input.
    assign system_reset_n =
        !(power_reset[2] || debug_reset);

endmodule