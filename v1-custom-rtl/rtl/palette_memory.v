module palette_memory (
    input  wire        clk,

    // CPU write port.
    input  wire        cpu_write,
    input  wire [8:0]  cpu_palette_address,
    input  wire [23:0] cpu_writedata,

    // VGA read port.
    input  wire        front_bank,
    input  wire [7:0]  pixel_index,
    output reg  [23:0] rgb
);

    // Two banks of 256 RGB entries.
    (* ramstyle = "M9K" *) reg [23:0] palettes [0:511];

    wire [8:0] scan_palette_address;

    assign scan_palette_address = {
        front_bank,
        pixel_index
    };

    always @(posedge clk) begin
        // Synchronous palette lookup: one clock-cycle latency.
        rgb <= palettes[scan_palette_address];

        if (cpu_write)
            palettes[cpu_palette_address] <= cpu_writedata;
    end

endmodule