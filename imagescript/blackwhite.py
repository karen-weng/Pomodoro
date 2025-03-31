from PIL import Image

# Load image
image_name = "school_bell_outline"
image_path = image_name + ".png"
img = Image.open(image_path).convert("RGB")  # Convert to RGB format

width, height = img.size
pixels = list(img.getdata())

# Convert pixels to black/white (white will be treated as transparent)
def rgb_to_bw(r, g, b):
    # If pixel is close to white, mark it as transparent (-1)
    if r > 200 and g > 200 and b > 200:
        return -1  # Transparent
    return 0x0000  # Black (pure black in 16-bit 565 format)

bw_pixel_array = [rgb_to_bw(r, g, b) for (r, g, b) in pixels]

# Save as a C array
header_filename = image_name + "_bw.h"
with open(header_filename, "w") as f:


    f.write(f"int {image_name}_width = {width};\n")
    f.write(f"int {image_name}_height = {height};\n")
    f.write("const short int " + image_name + "_data[] = {")

    f.write(", ".join(map(str, bw_pixel_array)))
    f.write("};")

print(f"Header file '{header_filename}' generated successfully.")