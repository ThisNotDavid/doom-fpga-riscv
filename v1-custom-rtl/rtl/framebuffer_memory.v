module framebuffer_memory (
    input  wire        clk,

    // CPU write port. Address includes the bank-selection bit.
    input  wire        cpu_write,
    input  wire [14:0] cpu_word_address,
    input  wire [31:0] cpu_writedata,
    input  wire [3:0]  cpu_byteenable,

    // VGA read port.
    input  wire        front_bank,
    input  wire [15:0] scan_pixel_address,
    output wire [7:0]  pixel_index
);

    // Each lane stores one byte of every 32-bit word.
    // Together: two banks, each containing 65536 bytes.
    (* ramstyle = "M9K" *) reg [7:0] lane0 [0:32767];
    (* ramstyle = "M9K" *) reg [7:0] lane1 [0:32767];
    (* ramstyle = "M9K" *) reg [7:0] lane2 [0:32767];
    (* ramstyle = "M9K" *) reg [7:0] lane3 [0:32767];

    wire [14:0] scan_word_address;

    reg [7:0] pixel0;
    reg [7:0] pixel1;
    reg [7:0] pixel2;
    reg [7:0] pixel3;
    reg [1:0] lane_select;

    assign scan_word_address = {
        front_bank,
        scan_pixel_address[15:2]
    };

    // Select the byte from the registered memory word.
    assign pixel_index =
        (lane_select == 2'd0) ? pixel0 :
        (lane_select == 2'd1) ? pixel1 :
        (lane_select == 2'd2) ? pixel2 :
                               pixel3;

    always @(posedge clk) begin
        // Synchronous VGA reads: one clock-cycle latency.
        pixel0 <= lane0[scan_word_address];
        pixel1 <= lane1[scan_word_address];
        pixel2 <= lane2[scan_word_address];
        pixel3 <= lane3[scan_word_address];

        // Delay the byte selection alongside the RAM data.
        lane_select <= scan_pixel_address[1:0];

        // Independent CPU write port.
        if (cpu_write) begin
            if (cpu_byteenable[0])
                lane0[cpu_word_address] <= cpu_writedata[7:0];

            if (cpu_byteenable[1])
                lane1[cpu_word_address] <= cpu_writedata[15:8];

            if (cpu_byteenable[2])
                lane2[cpu_word_address] <= cpu_writedata[23:16];

            if (cpu_byteenable[3])
                lane3[cpu_word_address] <= cpu_writedata[31:24];
        end
    end

endmodule