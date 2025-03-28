import numpy as np

if __name__ == "__main__":
    # Read raw PCM file
    filename = "rooster_alarm.raw"
    data = np.fromfile(filename, dtype=np.uint32)  # Read as 32-bit unsigned integers

    # Convert to C array format
    with open("samples_rooster_alarm.h", "w") as f:
        f.write("int samples[] = {\n")
        f.write(",\n".join(f"    0x{sample:08x}" for sample in data))
        f.write("\n};\n")
        f.write(f"int samples_n = {len(data)};\n")

        print("C array saved to rooster_alarm.h")