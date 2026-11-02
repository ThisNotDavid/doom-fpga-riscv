// 50 MHz, 32-bit SDR SDRAM: 4 banks x 8192 rows x 1024 columns.
// Conservative single-word transactions, CAS 2, auto-precharge.
// DRAM_CLK must be the inverted system clock. No burst interface: Platform
// Designer inserts the burst adapter for the cached Nios V/g masters.
module doom_sdram #(
    parameter integer INIT_CYCLES=11000,
    parameter integer REFRESH_CYCLES=350
)(
    input wire clk, reset,
    input wire [24:0] address,
    input wire read, write,
    input wire [31:0] writedata,
    input wire [3:0] byteenable,
    output wire waitrequest,
    output reg [31:0] readdata,
    output reg readdatavalid,
    output reg [12:0] dram_addr,
    output reg [1:0] dram_ba,
    output wire dram_cs_n, dram_cke,
    output reg dram_ras_n, dram_cas_n, dram_we_n,
    output reg [3:0] dram_dqm,
    inout wire [31:0] dram_dq,
    output reg ready
);
    localparam BOOT=0, PRE=1, INIT_REF=2, MODE=3, IDLE=4,
               ACT=5, ACCESS=6, READ_WAIT=7, RECOVER=8, REFRESH=9, DELAY=10;
    reg [3:0] state, next_state;
    reg [15:0] delay_count;
    reg [15:0] refresh_count;
    reg [3:0] init_refs;
    reg [24:0] saved_addr;
    reg [31:0] saved_data;
    reg [3:0] saved_be;
    reg saved_write, drive;
    reg [31:0] dq_sample;
    // Capture one full SDRAM clock after the CAS-2 data launch. This provides
    // substantially more input setup margin than half-cycle CPU-edge capture.
    always @(negedge clk) dq_sample<=dram_dq;
    assign dram_dq=drive ? saved_data : 32'bz;
    assign dram_cs_n=1'b0;
    assign dram_cke=1'b1;
    assign waitrequest=reset || state!=IDLE || refresh_count==0;
    always @(posedge clk) begin
        if(reset) begin
            state<=BOOT;delay_count<=INIT_CYCLES;refresh_count<=REFRESH_CYCLES;
            init_refs<=0;ready<=0;drive<=0;readdatavalid<=0;readdata<=0;
            dram_addr<=0;dram_ba<=0;dram_dqm<=0;
            dram_ras_n<=1;dram_cas_n<=1;dram_we_n<=1;
            saved_addr<=0;saved_data<=0;saved_be<=0;saved_write<=0;next_state<=IDLE;
        end else begin
            // Commands issued here are sampled on the following falling edge.
            dram_ras_n<=1;dram_cas_n<=1;dram_we_n<=1;
            drive<=0;dram_dqm<=0;readdatavalid<=0;
            if(ready && refresh_count!=0) refresh_count<=refresh_count-1'b1;
            case(state)
                BOOT: if(delay_count==0) state<=PRE; else delay_count<=delay_count-1'b1;
                PRE: begin
                    dram_ras_n<=0;dram_we_n<=0;dram_addr<=13'h400;
                    delay_count<=2;next_state<=INIT_REF;state<=DELAY;
                end
                INIT_REF: begin
                    dram_ras_n<=0;dram_cas_n<=0;init_refs<=init_refs+1'b1;
                    delay_count<=5;next_state<=init_refs==7 ? MODE : INIT_REF;state<=DELAY;
                end
                MODE: begin
                    dram_ras_n<=0;dram_cas_n<=0;dram_we_n<=0;
                    dram_ba<=0;dram_addr<=13'h220;
                    delay_count<=2;next_state<=IDLE;state<=DELAY;
                    ready<=1;refresh_count<=REFRESH_CYCLES;
                end
                IDLE: begin
                    if(refresh_count==0) state<=REFRESH;
                    else if(read || write) begin
                        saved_addr<=address;saved_data<=writedata;
                        saved_be<=byteenable;saved_write<=write;state<=ACT;
                    end
                end
                ACT: begin
                    dram_ras_n<=0;dram_ba<=saved_addr[24:23];dram_addr<=saved_addr[22:10];
                    delay_count<=1;next_state<=ACCESS;state<=DELAY;
                end
                ACCESS: begin
                    dram_cas_n<=0;dram_we_n<=!saved_write;
                    dram_ba<=saved_addr[24:23];dram_addr<={2'b0,1'b1,saved_addr[9:0]};
                    if(saved_write) begin
                        drive<=1;dram_dqm<=~saved_be;
                        delay_count<=5;next_state<=IDLE;state<=DELAY;
                    end else begin delay_count<=3;state<=READ_WAIT;end
                end
                READ_WAIT: if(delay_count!=0) delay_count<=delay_count-1'b1;
                    else begin
                        readdata<=dq_sample;readdatavalid<=1;
                        delay_count<=3;next_state<=IDLE;state<=DELAY;
                    end
                REFRESH: begin
                    dram_ras_n<=0;dram_cas_n<=0;refresh_count<=REFRESH_CYCLES;
                    delay_count<=5;next_state<=IDLE;state<=DELAY;
                end
                DELAY: if(delay_count==0) state<=next_state;else delay_count<=delay_count-1'b1;
                default: state<=BOOT;
            endcase
        end
    end
endmodule
