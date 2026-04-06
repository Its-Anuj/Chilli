import os
import subprocess
import time
import sys

def get_choice(prompt, options):
    print(f"\n{prompt}")
    for key, value in options.items():
        print(f"{key}. {value}")
    
    while True:
        choice = input("Enter number: ").strip()
        if choice in options:
            return choice
        print("Invalid choice. Please try again.")

def main():
    # 1. Select Build Type
    build_types = {"1": "Debug", "2": "Release", "3": "RelWithDebInfo"}
    bt_choice = get_choice("Select Build Type:", build_types)
    build_type = build_types[bt_choice]

    # 2. Select Build System (Generator)
    generators = {"1": "Ninja", "2": "Visual Studio 17 2022", "3": "MinGW Makefiles"}
    gen_choice = get_choice("Select Build System:", generators)
    generator = generators[gen_choice]

    # 3. Select Build Folder
    folders = {"1": "build", "2": "Custom"}
    folder_choice = get_choice("Select Build Folder:", folders)
    
    if folder_choice == "2":
        build_dir = input("Enter custom folder name: ").strip()
    else:
        build_dir = "build"

    # Create directory if it doesn't exist
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    # Prepare Commands
    # Note: We use -S . because the script is usually run from the root, 
    # but based on your prompt 'cmake -S ..', we assume you want to look one level up.
    configure_cmd = [
        "cmake",
        "-S", ".", 
        "-B", build_dir,
        "-G", generator,
        f"-DCMAKE_BUILD_TYPE={build_type}",
        "-DJPH_USE_DX12=0",
        "-DJPH_USE_DXC=0"
    ]

    build_cmd = ["cmake", "--build", build_dir, "--config", build_type]

    try:
        print("\n--- Starting Configuration ---")
        start_time = time.time()

        # Run Configuration
        subprocess.run(configure_cmd, check=True)

        print("\n--- Starting Build ---")
        # Run Build
        subprocess.run(build_cmd, check=True)

        end_time = time.time()
        duration = end_time - start_time

        print("\n" + "="*30)
        print("BUILD SUCCESSFUL")
        print(f"Total time: {duration:.2f} seconds")
        print("="*30)

    except subprocess.CalledProcessError as e:
        print(f"\nERROR: Build failed with exit code {e.returncode}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nBuild cancelled by user.")
        sys.exit(0)

if __name__ == "__main__":
    main()