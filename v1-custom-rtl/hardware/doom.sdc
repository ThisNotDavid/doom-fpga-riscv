# Main board clock: 50 MHz.
create_clock -name clk50 -period 20.000 [get_ports CLOCK_50]

# External SDRAM clock is the inverted system clock.
create_generated_clock \
    -name dram_clk \
    -source [get_ports CLOCK_50] \
    -divide_by 1 \
    -invert \
    [get_ports DRAM_CLK]

# VGA clock register moved into the vga_output submodule.
set vga_clock_register [get_registers {*|output_stage|vga_clk}]

if {[get_collection_size $vga_clock_register] != 1} {
    error "Expected exactly one VGA clock register; inspect the fitted hierarchy."
}

# Preserve the reference's forwarded VGA clock relationship.
create_generated_clock \
    -name vga_clk_internal \
    -source [get_ports CLOCK_50] \
    -edges {2 4 6} \
    $vga_clock_register

create_generated_clock \
    -name vga_clk \
    -source $vga_clock_register \
    -divide_by 1 \
    [get_ports VGA_CLK]

derive_clock_uncertainty

# Asynchronous board inputs.
# PS/2 inputs are synchronized inside ps2_receiver.
set_false_path -from [get_ports {KEY[0] PS2_CLK PS2_DAT}]

# Inherited provisional SDRAM device/PCB timing budgets.
set dram_outputs [get_ports {
    DRAM_ADDR[*]
    DRAM_BA[*]
    DRAM_CAS_N
    DRAM_RAS_N
    DRAM_WE_N
    DRAM_CS_N
    DRAM_CKE
    DRAM_DQM[*]
    DRAM_DQ[*]
}]

set_output_delay -clock dram_clk -max 2.0 $dram_outputs
set_output_delay -clock dram_clk -min -1.0 $dram_outputs

set_input_delay -clock dram_clk -max 6.5 [get_ports {DRAM_DQ[*]}]
set_input_delay -clock dram_clk -min 2.0 [get_ports {DRAM_DQ[*]}]

# VGA DAC data and control outputs.
set vga_outputs [get_ports {
    VGA_R[*]
    VGA_G[*]
    VGA_B[*]
    VGA_BLANK_N
    VGA_SYNC_N
}]

set_output_delay -clock vga_clk -max 2.0 $vga_outputs
set_output_delay -clock vga_clk -min -2.0 $vga_outputs

set_output_delay -clock clk50 -max 2.0 [get_ports {VGA_HS VGA_VS}]
set_output_delay -clock clk50 -min 0.0 [get_ports {VGA_HS VGA_VS}]