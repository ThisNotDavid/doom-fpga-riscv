module framebuffer_address (
    input  wire [9:0]  x,
    input  wire [9:0]  y,

    output wire       image_active,
    output wire [15:0] pixel_address
);

    wire [9:0] image_y;
    wire [8:0] source_x;
    wire [8:0] source_y;

    // Image occupies x = 0..639 and y = 40..439.
    assign image_active =
        (x < 10'd640) &&
        (y >= 10'd40) &&
        (y < 10'd440);

    assign image_y  = y - 10'd40;
    assign source_x = x[9:1];
    assign source_y = image_y[9:1];

    // Byte address of an indexed pixel: row * 320 + column.
    // Use address zero outside the image; output will be blanked.
    assign pixel_address = image_active
        ? (({7'd0, source_y} * 16'd320) + {7'd0, source_x})
        : 16'd0;

endmodule