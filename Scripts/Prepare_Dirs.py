#!/usr/bin/env python3
"""
Prepare required asset directories.
Usage:
    python Prepare_Dirs.py <dir1> [<dir2> ...]
Example:
    python Prepare_Dirs.py BinDir/Assets/Shaders BinDir/Assets/Textures BinDir/Assets/Models
"""

from pathlib import Path
import sys

def prepare_dir(path_str: str):
    path = Path(path_str)
    try:
        path.mkdir(parents=True, exist_ok=True)
        if path.exists():
            print(f"Exists:  {path.resolve()}")
    except Exception as e:
        print(f"Error creating {path}: {e}", file=sys.stderr)
        return False
    return True

def main():
    if len(sys.argv) < 2:
        print("Usage: python Prepare_Dirs.py <dir1> [<dir2> ...]")
        sys.exit(1)

    print("Preparing asset directories...")
    ok = True
    for dir_arg in sys.argv[1:]:
        if not prepare_dir(dir_arg):
            ok = False
    if not ok:
        sys.exit(2)

if __name__ == "__main__":
    main()
