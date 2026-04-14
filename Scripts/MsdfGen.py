import os
import subprocess
import sys

def process_fonts(raw_dir, out_dir, exe_path):
# Check if exe_path is a directory; if so, find the .exe inside it
    if os.path.isdir(exe_path):
        for root, dirs, files in os.walk(exe_path):
            for file in files:
                if file == "msdf-atlas-gen.exe":
                    exe_path = os.path.join(root, file)
                    break
    
    if not os.path.isfile(exe_path):
        print(f"!! Error: Could not find msdf-atlas-gen.exe at {exe_path}")
        return

    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    # Scour for TTF files
    for file in os.listdir(raw_dir):
        if file.lower().endswith(".ttf"):
            font_name = os.path.splitext(file)[0]
            input_path = os.path.join(raw_dir, file)
            output_json = os.path.join(out_dir, f"{font_name}.msdf")
            output_png = os.path.join(out_dir, f"{font_name}.png")

            # Only generate if input is newer than output (Optimization)
            if not os.path.exists(output_json) or os.path.getmtime(input_path) > os.path.getmtime(output_json):
                print(f"-- Generating MSDF for {file}...")
                
                cmd = [
                    exe_path,
                    "-font", input_path,
                    "-type", "msdf",
                    "-size", "32",
                    "-pxrange", "4",
                    "-yorigin", "top",
                    "-imageout", output_png,
                    "-json", output_json
                ]

                try:
                    subprocess.run(cmd, check=True)
                except subprocess.CalledProcessError as e:
                    print(f"!! Failed to generate font {file}: {e}")

if __name__ == "__main__":
    print(sys.argv[1])
    print(sys.argv[2])
    print(sys.argv[3])
    # Expects: script.py <raw_dir> <out_dir> <exe_path>
    process_fonts(sys.argv[1], sys.argv[2], sys.argv[3])