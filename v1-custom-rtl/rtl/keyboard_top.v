module keyboard_top (
    input  wire        clk,
    input  wire        reset,

    input  wire        ps2_clk,
    input  wire        ps2_dat,

    // Decoded requests from doom_io_bus.
    input  wire        pop,
    input  wire [1:0]  clear_errors,

    output wire [31:0] key_data,
    output wire [31:0] key_errors
);

    wire [7:0] received_data;
    wire       received_valid;
    wire       received_error;

    wire [7:0] head_data;
    wire       head_valid;
    wire       overflow;

    reg frame_error;

    ps2_receiver receiver (
        .clk         (clk),
        .reset       (reset),
        .ps2_clk     (ps2_clk),
        .ps2_data    (ps2_dat),
        .data        (received_data),
        .data_valid  (received_valid),
        .frame_error (received_error)
    );

    keyboard_fifo fifo (
        .clk            (clk),
        .reset          (reset),
        .push_data      (received_data),
        .push_valid     (received_valid),
        .pop            (pop),
        .clear_overflow (clear_errors[0]),
        .head_data      (head_data),
        .head_valid     (head_valid),
        .overflow       (overflow)
    );

    // Sticky receiver error, cleared by writing one.
    always @(posedge clk) begin
        if (reset) begin
            frame_error <= 1'b0;
        end else begin
            if (clear_errors[1])
                frame_error <= 1'b0;

            if (received_error)
                frame_error <= 1'b1;
        end
    end

    // Bit 8 indicates whether a byte is available.
    assign key_data = head_valid
        ? {23'd0, 1'b1, head_data}
        : 32'd0;

    // Bit 0: FIFO overflow. Bit 1: receiver frame error.
    assign key_errors = {30'd0, frame_error, overflow};

endmodule