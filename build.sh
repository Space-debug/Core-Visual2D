#!/bin/bash

# Visual2DInfer Build Script
# Usage: ./build.sh [clean|rebuild] [Debug|Release]

set -e  # Exit immediately if a command exits with a non-zero status

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Project configuration
PROJECT_NAME="Visual2DInfer"
BUILD_DIR="build"
BUILD_TYPE="Release"

# Parse arguments
CLEAN_BUILD=false
REBUILD=false

for arg in "$@"; do
    case $arg in
        clean)
            CLEAN_BUILD=true
            ;;
        rebuild)
            REBUILD=true
            CLEAN_BUILD=true
            ;;
        Debug|Release)
            BUILD_TYPE="$arg"
            ;;
        *)
            echo -e "${YELLOW}Unknown argument: $arg${NC}"
            echo "Usage: $0 [clean|rebuild] [Debug|Release]"
            exit 1
            ;;
    esac
done

# Print build information
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  ${PROJECT_NAME} Build Script${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "Build Type: ${YELLOW}${BUILD_TYPE}${NC}"
echo -e "Build Directory: ${YELLOW}${BUILD_DIR}${NC}"
echo ""

# Clean build directory
if [ "$CLEAN_BUILD" = true ] || [ "$REBUILD" = true ]; then
    if [ -d "$BUILD_DIR" ]; then
        echo -e "${YELLOW}Cleaning build directory: ${BUILD_DIR}${NC}"
        rm -rf "$BUILD_DIR"
    fi
    if [ -d "_LIBRARY" ]; then
        echo -e "${YELLOW}Cleaning output directory: _LIBRARY${NC}"
        rm -rf "_LIBRARY"
    fi
    echo -e "${GREEN}Clean completed${NC}"
    echo ""
fi

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${GREEN}Creating build directory: ${BUILD_DIR}${NC}"
    mkdir -p "$BUILD_DIR"
fi

# Enter build directory
cd "$BUILD_DIR"

# Configure CMake
echo -e "${GREEN}Configuring CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
fi

# Build
echo ""
echo -e "${GREEN}Starting build...${NC}"
echo ""

# Get CPU core count for parallel compilation
if command -v nproc &> /dev/null; then
    JOBS=$(nproc)
else
    JOBS=4
fi

cmake --build . --config "${BUILD_TYPE}" -j "${JOBS}"

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Return to project root directory
cd ..

# Check output
if [ -d "_LIBRARY" ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  Build successful!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo -e "Output directory: ${GREEN}_LIBRARY/${NC}"
    echo ""
    echo "Directory structure:"
    ls -lh _LIBRARY/ 2>/dev/null || true
    if [ -d "_LIBRARY/lib" ]; then
        echo ""
        echo "Generated library files:"
        ls -lh _LIBRARY/lib/*.so 2>/dev/null || ls -lh _LIBRARY/lib/*.a 2>/dev/null || true
    fi
else
    echo -e "${YELLOW}Warning: Output directory _LIBRARY not found${NC}"
fi

echo ""
echo -e "${GREEN}Build completed!${NC}"

