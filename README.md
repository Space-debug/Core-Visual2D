# Visual2DInfer

A high-performance 2D visual inference library based on TensorRT, supporting object detection and segmentation tasks with multi-threaded processing capabilities.

## Features

- 🚀 **High Performance**: Built on TensorRT for optimized GPU inference
- 🎯 **Multi-Task Support**: Supports object detection and segmentation tasks
- 🔄 **Multi-Threaded**: Configurable thread pool for parallel inference
- 📦 **Easy Integration**: Simple C++ API with unified result interface
- 🛠️ **Build System**: CMake-based build with one-click build script
- 🧪 **Test Programs**: Included test programs for detection and segmentation

## Requirements

### System Requirements
- Linux (tested on Ubuntu)
- CUDA 12.6+ (or compatible version)
- CMake 3.18+
- C++17 compatible compiler (GCC 11+)

### Dependencies
- **TensorRT**: For model inference
- **CUDA**: For GPU acceleration
- **OpenCV**: Required for test programs (optional for library itself)
- **OpenMP**: Optional, for parallel processing in test utilities

## Project Structure

```
Core-Visual2D/
├── CMakeLists.txt          # Main CMake configuration
├── build.sh                # One-click build script
├── Config.cmake.in         # CMake package configuration template
├── include/                # Public headers
│   └── Visual2DInfer.h     # Main API header
├── src/                    # Source files
│   └── Visual2DInfer.cpp  # Implementation
├── test/                   # Test programs
│   ├── CMakeLists.txt      # Test build configuration
│   ├── detect.cpp          # Detection test program
│   ├── segment.cpp         # Segmentation test program
│   ├── convert.h           # Image conversion utilities
│   └── convert.cpp
├── 3rdparty/              # Third-party dependencies
│   ├── include/           # Third-party headers
│   └── lib/              # Third-party libraries
├── models/                # Model files directory
└── _LIBRARY/             # Build output directory (generated)
    ├── include/          # Copied headers
    └── lib/             # Compiled libraries
```

## Quick Start

### 1. Build the Project

#### Using the Build Script (Recommended)

```bash
# Build Release version (default)
./build.sh

# Build Debug version
./build.sh Debug

# Clean build directory
./build.sh clean

# Rebuild from scratch
./build.sh rebuild

# Build with test programs
./build.sh test

# Combine options: rebuild Debug version with tests
./build.sh rebuild Debug test
```

#### Using CMake Directly

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 2. Build Output

After building, the library and headers will be copied to `_LIBRARY/`:

```
_LIBRARY/
├── include/
│   ├── Visual2DInfer/
│   │   └── Visual2DInfer.h
│   └── deploy/
│       └── trtyolo.hpp
└── lib/
    ├── libVisual2DInfer.so
    └── [3rdparty libraries]
```

### 3. Use in Your Project

#### CMake Integration

```cmake
find_package(Visual2DInfer REQUIRED)
target_link_libraries(your_target PRIVATE Visual2DInfer)
```

#### Manual Integration

```cpp
#include "Visual2DInfer.h"

// Create inference engine
Visual2DInfer infer("path/to/model.engine", 4); // 4 threads

// Submit task
trtyolo::Image img(image_data, width, height);
infer.submitTask(img, frame_id);

// Get result
UnifiedResult result;
if (infer.getResult(result, frame_id)) {
    if (result.isDetect()) {
        auto& detect_res = result.asDetect();
        // Process detection results
    } else if (result.isSegment()) {
        auto& segment_res = result.asSegment();
        // Process segmentation results
    }
}
```

## API Reference

### Visual2DInfer Class

#### Constructor

```cpp
Visual2DInfer(const std::string& engine_file, uint8_t thread_num)
```

- `engine_file`: Path to TensorRT engine file (.engine)
- `thread_num`: Number of inference threads (typically 1-4)

#### Methods

```cpp
// Submit an inference task
void submitTask(const trtyolo::Image& img, int64_t id);

// Get inference result (blocking)
bool getResult(UnifiedResult& result, int64_t id);
```

#### Properties

```cpp
std::atomic<bool> stop_;      // Stop flag for graceful shutdown
TaskType task_type_;          // Inferred task type (DETECT/SEGMENT)
```

### UnifiedResult Structure

```cpp
struct UnifiedResult {
    TaskType type;  // DETECT or SEGMENT
    std::variant<trtyolo::DetectRes, trtyolo::SegmentRes> data;
    
    bool isDetect() const;
    bool isSegment() const;
    trtyolo::DetectRes& asDetect();
    trtyolo::SegmentRes& asSegment();
};
```

## Build Options

### CMake Options

- `BUILD_TEST_PROGRAMS`: Build test programs (default: OFF)
  ```bash
  cmake -DBUILD_TEST_PROGRAMS=ON ..
  ```

- `CMAKE_BUILD_TYPE`: Build type (Debug, Release, RelWithDebInfo, MinSizeRel)
  ```bash
  cmake -DCMAKE_BUILD_TYPE=Debug ..
  ```

### Build Types

- **Release**: Optimized build with `-O3`, no debug symbols
- **Debug**: Debug build with `-g -O0`, includes debug symbols
- **RelWithDebInfo**: Optimized with debug info (`-O2 -g`)
- **MinSizeRel**: Size-optimized build (`-Os`)

## Test Programs

### Detection Test

```bash
# Build test programs
./build.sh test

# Run detection test
cd build/test
./test_detect
```

The test program processes video frames and performs object detection.

### Segmentation Test

```bash
# Run segmentation test
cd build/test
./test_segment
```

The test program processes video frames and performs instance segmentation.

### Test Program Configuration

Test programs can be configured by modifying:
- Model path: `test/detect.cpp` or `test/segment.cpp`
- Video path: `test/detect.cpp` or `test/segment.cpp`
- Thread count: `test/detect.cpp` or `test/segment.cpp`

## Model Requirements

- **Format**: TensorRT engine files (.engine)
- **Naming Convention**: 
  - Detection models: `*.engine` (default) or models without "seg"/"mask" in name
  - Segmentation models: Models with "seg" or "mask" in filename

Example:
- `yolo11n.engine` → Detection model
- `yolo11n-seg.engine` → Segmentation model

## CUDA Configuration

The project is configured for CUDA architecture 8.7 (sm_87) by default. To change:

```cmake
set(CMAKE_CUDA_ARCHITECTURES "75" CACHE STRING "CUDA architecture")
```

## Troubleshooting

### Build Issues

1. **CUDA not found**
   - Ensure CUDA is installed and `CUDA_PATH` is set
   - Check `nvcc --version`

2. **TensorRT not found**
   - Ensure TensorRT libraries are in `3rdparty/lib/`
   - Check library paths in CMakeLists.txt

3. **OpenCV not found (for tests)**
   - Install OpenCV: `sudo apt-get install libopencv-dev`
   - Or disable tests: `./build.sh` (without `test`)

### Runtime Issues

1. **Library not found**
   - Ensure `_LIBRARY/lib/` is in `LD_LIBRARY_PATH`
   - Or use RPATH: libraries are configured with `$ORIGIN` RPATH

2. **CUDA out of memory**
   - Reduce batch size or number of threads
   - Use smaller model or lower input resolution

## Development

### Code Structure

- **include/Visual2DInfer.h**: Main API definition
- **src/Visual2DInfer.cpp**: Implementation with thread pool and task queue
- **test/**: Test programs and utilities

### Adding New Task Types

1. Add task type to `TaskType` enum in `Visual2DInfer.h`
2. Add model variant in `models_` vector
3. Update `inferTaskType()` to detect new task type
4. Add result type to `UnifiedResult::data` variant

## License

[Specify your license here]

## Contributing

[Contributing guidelines if applicable]

## Contact

[Contact information]

## Version

Current version: 1.0.0

