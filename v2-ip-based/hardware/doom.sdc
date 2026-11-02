# DE2-115 onboard oscillator: 50 MHz.
create_clock -name CLOCK_50 -period 20.000 [get_ports {CLOCK_50}]

derive_clock_uncertainty

derive_pll_clocks

# Pushbutton is an asynchronous reset source.
# Reset release is synchronized inside the generated system.
set_false_path -from [get_ports {KEY[0]}]

# LEDs are human-visible outputs with no external capture clock.
set_false_path -to [get_ports {LEDR[*]}]