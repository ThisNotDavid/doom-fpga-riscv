module ps2_receiver #(
    parameter integer FILTER_BITS    = 8,
    parameter integer TIMEOUT_CYCLES = 100000
)(
    input  wire       clk,
    input  wire       reset,
    input  wire       ps2_clk,
    input  wire       ps2_data,

    output reg  [7:0] data,
    output reg        data_valid,
    output reg        frame_error
);

    // Verilog-2001 constant function replacing $clog2.
    function integer bits_required;
        input integer value;
        integer remaining;
        begin
            remaining = value - 1;
            bits_required = 0;

            while (remaining > 0) begin
                bits_required = bits_required + 1;
                remaining = remaining >> 1;
            end

            if (bits_required < 1)
                bits_required = 1;
        end
    endfunction

    localparam integer TIMER_BITS =
        bits_required(TIMEOUT_CYCLES + 1);

    (* altera_attribute =
       "-name SYNCHRONIZER_IDENTIFICATION FORCED_IF_ASYNCHRONOUS" *)
    reg [1:0] clock_sync = 2'b11;
    (* altera_attribute =
       "-name SYNCHRONIZER_IDENTIFICATION FORCED_IF_ASYNCHRONOUS" *)
    reg [1:0] data_sync = 2'b11;

    reg [FILTER_BITS-1:0] clock_history;
    reg                   filtered_clock;
    reg                   previous_clock;

    reg [3:0] bit_count;
    reg [7:0] shift;
    reg       parity_bit;

    reg [TIMER_BITS-1:0] timer;

    wire falling;

    assign falling = previous_clock && !filtered_clock;

    always @(posedge clk) begin
        clock_sync <= {clock_sync[0], ps2_clk};
        data_sync  <= {data_sync[0], ps2_data};

        if (reset) begin
            clock_history  <= {FILTER_BITS{1'b1}};
            filtered_clock <= 1'b1;
            previous_clock <= 1'b1;

            bit_count  <= 4'd0;
            shift      <= 8'd0;
            parity_bit <= 1'b0;
            timer      <= {TIMER_BITS{1'b0}};

            data        <= 8'd0;
            data_valid  <= 1'b0;
            frame_error <= 1'b0;
        end else begin
            clock_history <= {
                clock_history[FILTER_BITS-2:0],
                clock_sync[1]
            };

            if (&clock_history)
                filtered_clock <= 1'b1;
            else if (~|clock_history)
                filtered_clock <= 1'b0;

            previous_clock <= filtered_clock;

            data_valid  <= 1'b0;
            frame_error <= 1'b0;

            // Abort an incomplete frame after 2 ms at 50 MHz.
            if (bit_count == 4'd0) begin
                timer <= {TIMER_BITS{1'b0}};
            end else if (timer == TIMEOUT_CYCLES - 1) begin
                bit_count  <= 4'd0;
                timer      <= {TIMER_BITS{1'b0}};
                frame_error <= 1'b1;
            end else begin
                timer <= timer + 1'b1;
            end

            if (falling) begin
                timer <= {TIMER_BITS{1'b0}};

                case (bit_count)
                    4'd0: begin
                        // Start bit must be zero.
                        if (!data_sync[1])
                            bit_count <= 4'd1;
                    end

                    4'd1, 4'd2, 4'd3, 4'd4,
                    4'd5, 4'd6, 4'd7, 4'd8: begin
                        shift[bit_count - 4'd1] <= data_sync[1];
                        bit_count <= bit_count + 4'd1;
                    end

                    4'd9: begin
                        parity_bit <= data_sync[1];
                        bit_count  <= 4'd10;
                    end

                    4'd10: begin
                        bit_count <= 4'd0;

                        // Stop bit must be one; parity must be odd.
                        if (data_sync[1] &&
                            ((^shift) ^ parity_bit)) begin
                            data       <= shift;
                            data_valid <= 1'b1;
                        end else begin
                            frame_error <= 1'b1;
                        end
                    end

                    default: begin
                        bit_count <= 4'd0;
                    end
                endcase
            end
        end
    end

endmodule