# ...existing code...
#!/usr/bin/env python3
r"""
Compile all .vert and .frag GLSL files found in an input path using glslc,
produce outputs named <shadername>_vert.spv and <shadername>_frag.spv and copy
them to a provided output directory.

Usage:
    python Shader_Compile.py <input_path> <output_dir> [--glslc C:\path\to\glslc.exe]
"""
from pathlib import Path
import argparse
import shutil
import subprocess
import sys

def find_shader_files(src: Path):
    files = []
    if src.is_file():
        if src.suffix in ('.vert', '.frag'):
            files.append(src)
    else:
        files.extend(src.rglob('*.vert'))
        files.extend(src.rglob('*.frag'))
    return sorted(set(files))

def mapped_out_name(src: Path):
    stem = src.stem  # removes one suffix, e.g. "shader" from "shader.vert"
    if src.suffix == '.vert':
        return f"{stem}_vert.spv"
    if src.suffix == '.frag':
        return f"{stem}_frag.spv"
    return f"{stem}.spv"

def which_exe(name):
    # simple check for glslc in PATH or provided absolute path
    exe = shutil.which(name)
    return exe

def compile_shader(glslc_path: str, src: Path, dest: Path):
    dest.parent.mkdir(parents=True, exist_ok=True)
    cmd = [glslc_path, str(src), '-o', str(dest)]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
    except FileNotFoundError:
        return False, f"glslc executable not found: {glslc_path}"
    if proc.returncode != 0:
        return False, proc.stderr.strip() or proc.stdout.strip()
    return True, ""

def main():
    p = argparse.ArgumentParser(description="Compile .vert/.frag files with glslc and copy .spv outputs")
    p.add_argument("input", help="Input file or directory containing .vert/.frag files")
    p.add_argument("output", help="Directory to place compiled .spv files")
    p.add_argument("--glslc", help="Path to glslc executable (optional; must be in PATH if omitted)")
    args = p.parse_args()

    src = Path(args.input).resolve()
    outdir = Path(args.output).resolve()
    outdir.mkdir(parents=True, exist_ok=True)

    glslc_path = args.glslc or "glslc"
    if args.glslc is None:
        found = which_exe("glslc")
        if not found:
            print("Error: glslc not found in PATH. Provide --glslc <path_to_glslc.exe>", file=sys.stderr)
            sys.exit(2)
        glslc_path = found

    shader_files = find_shader_files(src)
    if not shader_files:
        print("No .vert or .frag files found.", file=sys.stderr)
        sys.exit(1)

    failed = []
    succeeded = []
    for s in shader_files:
        out_name = mapped_out_name(s)
        dest = outdir / out_name

        # Skip recompiling if output exists and is newer than input
        if dest.exists() and dest.stat().st_mtime >= s.stat().st_mtime:
            print(f"Up to date: {s}")
            continue

        ok, msg = compile_shader(glslc_path, s, dest)
        if ok:
            succeeded.append((s, dest))
            print(f"Compiled: {s} -> {dest}")
        else:
            failed.append((s, msg))
            print(f"Failed: {s}: {msg}", file=sys.stderr)

    print(f"\nSummary: {len(succeeded)} succeeded, {len(failed)} failed.")
    if failed:
        sys.exit(3)

if __name__ == "__main__":
    main()
# ...existing code...