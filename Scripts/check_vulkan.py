import os
import platform
import subprocess
import sys
import urllib.request
import tarfile
import tempfile
import shutil

MIN_VERSION = (1, 3, 0)  # Minimum required Vulkan version

def parse_vulkan_version(output: str):
    """Extract the Vulkan API version from vulkaninfo output."""
    import re
    match = re.search(r"Vulkan Instance Version:\s+(\d+)\.(\d+)\.(\d+)", output)
    if match:
        return tuple(map(int, match.groups()))
    return None

def is_version_supported(version, minimum):
    return version >= minimum

def run_command(cmd):
    try:
        return subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, text=True)
    except subprocess.CalledProcessError:
        return None

def check_vulkan_support():
    print("🔍 Checking for Vulkan support...")
    vulkaninfo_output = run_command("vulkaninfo")
    if not vulkaninfo_output:
        print("❌ Vulkan not found on this system.")
        return False

    version = parse_vulkan_version(vulkaninfo_output)
    if not version:
        print("⚠️ Couldn't detect Vulkan version.")
        return False

    print(f"✅ Vulkan {version[0]}.{version[1]}.{version[2]} detected.")
    if is_version_supported(version, MIN_VERSION):
        print("🎉 Vulkan 1.3 or higher supported.")
        return True
    else:
        print("❌ Vulkan version too low.")
        return False

def check_vulkan_sdk():
    print("🔍 Checking Vulkan SDK installation...")
    sdk_path = os.environ.get("VULKAN_SDK")
    if not sdk_path:
        print("⚠️ VULKAN_SDK environment variable not set.")
        return None

    version_file = os.path.join(sdk_path, "VERSION.txt")
    if os.path.exists(version_file):
        with open(version_file, "r") as f:
            version = tuple(map(int, f.read().strip().split(".")))
            print(f"✅ Vulkan SDK version {version} found.")
            if is_version_supported(version, MIN_VERSION):
                print("🎉 Vulkan SDK is up-to-date.")
                return True
            else:
                print("❌ Vulkan SDK is outdated.")
                return False
    else:
        print("⚠️ Vulkan SDK not properly installed.")
        return False

def download_and_install_sdk():
    system = platform.system()
    print(f"⬇️ Downloading Vulkan SDK for {system}...")

    if system == "Windows":
        url = "https://sdk.lunarg.com/sdk/download/latest/windows/vulkan-sdk.exe"
        installer = os.path.join(tempfile.gettempdir(), "vulkan-sdk.exe")
        urllib.request.urlretrieve(url, installer)
        print("📦 Running Vulkan SDK installer...")
        os.system(f'start /wait "" "{installer}" /S')  # silent install
        print("✅ Vulkan SDK installed successfully.")

    elif system == "Linux":
        url = "https://sdk.lunarg.com/sdk/download/latest/linux/vulkan-sdk.tar.gz"
        tar_path = os.path.join(tempfile.gettempdir(), "vulkan-sdk.tar.gz")
        urllib.request.urlretrieve(url, tar_path)
        print("📦 Extracting Vulkan SDK...")
        extract_path = "/opt/vulkan-sdk"
        os.makedirs(extract_path, exist_ok=True)
        with tarfile.open(tar_path, "r:gz") as tar:
            tar.extractall(extract_path)
        print(f"✅ Vulkan SDK installed to {extract_path}.")
        print(f"🔧 Add this to your shell config: export VULKAN_SDK={extract_path}/x.x.x.x/x86_64")

    else:
        print("❌ Unsupported OS for automatic SDK installation.")
        return False
    return True

def main():
    print("=== Vulkan 1.3 Compatibility Check ===\n")

    supported = check_vulkan_support()
    if not supported:
        print("🚫 Vulkan 1.3 not supported. Your GPU or drivers may be too old.")
        sys.exit(1)

    sdk_ok = check_vulkan_sdk()
    if sdk_ok is True:
        print("✅ All good! Vulkan SDK 1.3+ already installed.")
    else:
        print("⚙️ Installing Vulkan SDK 1.3+ ...")
        if download_and_install_sdk():
            print("🎉 Vulkan SDK installed and ready!")
        else:
            print("❌ Failed to install Vulkan SDK automatically.")

if __name__ == "__main__":
    main()
