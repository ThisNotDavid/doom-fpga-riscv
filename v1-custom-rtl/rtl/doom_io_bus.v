module doom_io_bus (
    input  wire        clk,
    input  wire        reset,

    // Avalon-MM interface.
    input  wire [15:0] address,
    input  wire        read,
    input  wire        write,
    input  wire [31:0] writedata,
    input  wire [3:0]  byteenable,
    output reg  [31:0] readdata,

    // Peripheral read data.
    input  wire [31:0] display_status,
    input  wire [31:0] milliseconds,
    input  wire [31:0] key_data,
    input  wire [31:0] key_errors,

    // Decoded peripheral requests.
    output wire        fb_write,
    output wire        palette_write,
    output wire        submit,
    output wire        key_pop,
    output wire [1:0]  key_clear_errors
);

    // All constants below are 32-bit WORD addresses.
    localparam [15:0] ADDR_PALETTE_BEGIN = 16'h8000;
    localparam [15:0] ADDR_PALETTE_END   = 16'h8200;

    localparam [15:0] ADDR_STATUS = 16'h8400;
    localparam [15:0] ADDR_SUBMIT = 16'h8401;
    localparam [15:0] ADDR_MS     = 16'h8402;
    localparam [15:0] ADDR_KEY    = 16'h8403;
    localparam [15:0] ADDR_ERRORS = 16'h8404;
    localparam [15:0] ADDR_ID     = 16'h8405;

    assign fb_write =
        !reset &&
        write &&
        (address < ADDR_PALETTE_BEGIN);

    assign palette_write =
        !reset &&
        write &&
        (address >= ADDR_PALETTE_BEGIN) &&
        (address < ADDR_PALETTE_END);

    assign submit =
        !reset &&
        write &&
        byteenable[0] &&
        (address == ADDR_SUBMIT);

    // The FIFO itself suppresses a pop when empty.
    assign key_pop =
        !reset &&
        read &&
        (address == ADDR_KEY);

    // Write-one-to-clear, enabled only for the low byte.
    assign key_clear_errors =
        (!reset &&
         write &&
         byteenable[0] &&
         (address == ADDR_ERRORS))
        ? writedata[1:0]
        : 2'b00;

    // One-cycle read latency, without waitrequest or readdatavalid.
    always @(posedge clk) begin
        if (reset) begin
            readdata <= 32'd0;
        end else begin
            readdata <= 32'd0;

            if (read) begin
                case (address)
                    ADDR_STATUS:
                        readdata <= display_status;

                    ADDR_MS:
                        readdata <= milliseconds;

                    ADDR_KEY:
                        readdata <= key_data;

                    ADDR_ERRORS:
                        readdata <= key_errors;

                    ADDR_ID:
                        readdata <= 32'h444F4F4D;

                    default:
                        readdata <= 32'd0;
                endcase
            end
        end
    end

endmodule