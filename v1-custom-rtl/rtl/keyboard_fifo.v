module keyboard_fifo (
    input  wire       clk,
    input  wire       reset,

    input  wire [7:0] push_data,
    input  wire       push_valid,
    input  wire       pop,
    input  wire       clear_overflow,

    output wire [7:0] head_data,
    output wire       head_valid,
    output reg        overflow
);

    reg [7:0] memory [0:255];
    reg [7:0] write_pointer;
    reg [7:0] read_pointer;
    reg [8:0] count;

    wire do_pop;
    wire do_push;

    assign head_valid = (count != 9'd0);
    assign head_data  = head_valid
        ? memory[read_pointer]
        : 8'd0;

    assign do_pop = pop && head_valid;

    assign do_push =
        push_valid && ((count != 9'd256) || do_pop);

    always @(posedge clk) begin
        if (reset) begin
            write_pointer <= 8'd0;
            read_pointer  <= 8'd0;
            count         <= 9'd0;
            overflow      <= 1'b0;
        end else begin
            if (clear_overflow)
                overflow <= 1'b0;

            // A new error takes priority over clearing the flag.
            if (push_valid && !do_push)
                overflow <= 1'b1;

            if (do_push) begin
                memory[write_pointer] <= push_data;
                write_pointer <= write_pointer + 8'd1;
            end

            if (do_pop)
                read_pointer <= read_pointer + 8'd1;

            case ({do_push, do_pop})
                2'b10: count <= count + 9'd1;
                2'b01: count <= count - 9'd1;
                default: ;
            endcase
        end
    end

endmodule