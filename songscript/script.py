import numpy as np

if __name__ == "__main__":
    # Read raw PCM file
    filename_beginning = "boo_44100"
    filename = filename_beginning+".raw"
    data = np.fromfile(filename, dtype=np.uint32)  # Read as 32-bit unsigned integers

    # Convert to C array format
    with open("samples_"+filename_beginning+".h", "w") as f:

        f.write(f"int {filename_beginning}_index = 0;\n")
        f.write("int "+filename_beginning+"_samples[] = {\n")

        f.write(",\n".join(f"    0x{sample:08x}" for sample in data))
        f.write("\n};\n")
        f.write(f"int {filename_beginning}_num_samples = {len(data)};\n")

        print("C array saved to", filename_beginning)