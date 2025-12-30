import pandas as pd
import matplotlib.pyplot as plt
import glob
import os

color_map = {
    "blue": "blue",
    "cyan": "cyan",
    "green": "green",
    "magenta": "magenta",
    "mono": "gray",
    "red": "red",
    "yellow": "yellow"
}

csv_files = glob.glob("QE_*.csv")

data = {}

for file in csv_files:
    color_name = os.path.splitext(os.path.basename(file))[0].split("_")[1].lower()
    if color_name in color_map:
        df = pd.read_csv(file, header=None, names=["wavelength", "value"])
        data[color_name] = df

# Plot each color individually
for color_name, df in data.items():
    plt.figure(figsize=(10, 6))
    plt.plot(df["wavelength"], df["value"], label=color_name, color=color_map[color_name])
    plt.xlabel("Wavelength (nm)")
    plt.ylabel("QE")
    plt.title("Quantum Efficiency per Color")
    plt.legend()
    plt.savefig(f"{color_name}.svg")
    plt.show()

# Combined plot for RGB
plt.figure(figsize=(10, 6))
for color_name in ["red", "green", "blue"]:
    if color_name in data:
        df = data[color_name]
        plt.plot(df["wavelength"], df["value"], label=color_name, color=color_map[color_name])
plt.xlabel("Wavelength (nm)")
plt.ylabel("QE")
plt.title("RGB Combined Plot")
plt.legend()
plt.savefig("RGB.svg")
plt.show()

# Combined plot for CMY
plt.figure(figsize=(10, 6))
for color_name in ["cyan", "magenta", "yellow"]:
    if color_name in data:
        df = data[color_name]
        plt.plot(df["wavelength"], df["value"], label=color_name, color=color_map[color_name])
plt.xlabel("Wavelength (nm)")
plt.ylabel("QE")
plt.title("CMY Combined Plot")
plt.legend()
plt.savefig("CMY.svg")
plt.show()
