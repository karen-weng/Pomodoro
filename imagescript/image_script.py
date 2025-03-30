from PIL import Image

# Load image
image_name = "mute_denoise"
image_path = image_name+".png"
img = Image.open(image_path).convert("RGB")  # Convert to RGB format

width, height = img.size
pixels = list(img.getdata())

# Convert pixels to 16-bit (5-6-5 format)
def rgb_to_565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

pixel_array = [rgb_to_565(r, g, b) for (r, g, b) in pixels]

# Save as a C array
with open(image_name+".h", "w") as f:
    f.write(f"#define IMAGE_WIDTH {width}\n")
    f.write(f"#define IMAGE_HEIGHT {height}\n")
    f.write("const short int image_data[] = {")
    f.write(", ".join(map(str, pixel_array)))
    f.write("};")
