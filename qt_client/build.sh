#!/bin/bash
# Build script for IM Client Qt GUI

set -e

echo "[INFO] Building IM Client Qt GUI..."

# Create build directory
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure with CMake
echo "[INFO] Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="/Users/z2835663859/Qt/6.5.3/macos" \
    -DCMAKE_FRAMEWORK_PATH="/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks"

# Pre-build: Fix link.txt for AGL framework issue (macOS)
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "[INFO] Applying macOS AGL framework workaround..."
    find . -name "link.txt" -exec sed -i '' 's/ -framework AGL//g' {} \; 2>/dev/null || true
fi

# Build
echo "[INFO] Building..."
cmake --build . --config Release -j4

# Check result
if [ -f "bin/im_client_gui" ]; then
    echo "[INFO] ✅ Build successful!"
    echo "[INFO] Executable: $(pwd)/bin/im_client_gui"
else
    echo "[ERROR] ❌ Build failed!"
    exit 1
fi
