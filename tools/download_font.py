import urllib.request
import zipfile
import io
import os

url = "https://github.com/IdreesInc/Monocraft/releases/download/v4.2.1/Monocraft-ttf.zip"
print("Downloading zip...")
req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
response = urllib.request.urlopen(req)
zip_data = response.read()

print("Extracting...")
ttf_data = None
with zipfile.ZipFile(io.BytesIO(zip_data)) as z:
    for name in z.namelist():
        if "Monocraft.ttf" in name:
            ttf_data = z.read(name)
            break

if not ttf_data:
    print("Could not find Monocraft.ttf in zip")
    exit(1)

print("Generating C header...")
os.makedirs("src/ui", exist_ok=True)
out = "const unsigned int Monocraft_ttf_len = {};\n".format(len(ttf_data))
out += "const unsigned char Monocraft_ttf[] = {\n"
out += ", ".join("0x{:02x}".format(b) for b in ttf_data)
out += "\n};\n"

with open("src/ui/Monocraft.h", "w") as f:
    f.write(out)

print("Done! Monocraft.h has been generated in src/ui/")
