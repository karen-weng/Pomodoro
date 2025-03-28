from pydub import AudioSegment
import numpy as np

# Load the song (supports MP3, WAV, etc.)
song = AudioSegment.from_file("testsong.mp3", format="mp3")

# Convert to mono (if stereo)
song = song.set_channels(1)

# Convert to 8kHz sampling rate (matches your FPGA)
song = song.set_frame_rate(8000)

# Get raw PCM data (signed 16-bit integers)
samples = np.array(song.get_array_of_samples(), dtype=np.int16)

# Normalize and scale to match FPGA 24-bit audio range
scaled_samples = (samples / np.max(np.abs(samples)) * 0x7FFFFF).astype(np.int32)

# Convert to hex format for inclusion in C code
samples_hex = [f"0x{val & 0xFFFFFF:06X}" for val in scaled_samples]  # Mask to 24-bit

# Save to a text file (C array format)
with open("audio_samples.txt", "w") as f:
    f.write(",\n    ".join(samples_hex))

print("Conversion complete! List saved to audio_samples.txt")
