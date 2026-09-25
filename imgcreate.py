import cairosvg
import os
import xml.etree.ElementTree as ET

ET.register_namespace("", "http://www.w3.org/2000/svg")  # prevents ns0:

# SVG file
svg_file = "calender.svg"

# Load SVG
tree = ET.parse(svg_file)
root = tree.getroot()

# Apply scale and translate to all rects inside <g>
scale = 0.82
size = 512 * scale
translate = (512 - size) / 2

inner_scale = 0.9
inner_size = 512 * inner_scale
inner_translate = (512 - inner_size) / 2

for g in root.findall("./{http://www.w3.org/2000/svg}g"):
    attr = "transform"
    if g.get(attr):
        g.set(attr, f"translate({translate}, {translate}) scale({scale})")
        for g in g.findall("./{http://www.w3.org/2000/svg}g"):
            if g.get(attr):
                g.set(attr, f"translate({inner_translate}, {inner_translate}) scale({inner_scale})")

# Save modified SVG
tree.write(svg_file)

# Base sizes for macOS iconset
sizes = [16, 32, 64, 128, 256, 512]

# Make folders
iconset_folder = "Budget.iconset"
os.makedirs(iconset_folder, exist_ok=True)
os.makedirs("assets", exist_ok=True)

# Generate PNGs
for size in sizes:
    # Base PNG
    cairosvg.svg2png(url=svg_file, write_to=f"{iconset_folder}/icon_{size}x{size}.png", output_width=size, output_height=size)

    # Retina PNG (@2x)
    retina_size = size * 2
    cairosvg.svg2png(url=svg_file, write_to=f"{iconset_folder}/icon_{size}x{size}@2x.png", output_width=retina_size, output_height=retina_size)

# Convert iconset to .icns in assets/
os.system("iconutil -c icns -o assets/Budget.icns Budget.iconset")
