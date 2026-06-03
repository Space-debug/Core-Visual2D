#!/bin/bash
# Build Core-Visual2D library and ROS2 node package.

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
ROS2_WS="${SCRIPT_DIR}"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Core-Visual2D ROS2 Build${NC}"
echo -e "${GREEN}========================================${NC}"

if [ ! -f /opt/ros/humble/setup.bash ]; then
    echo -e "${RED}ROS2 Humble not found at /opt/ros/humble/setup.bash${NC}"
    exit 1
fi

source /opt/ros/humble/setup.bash

# Avoid Anaconda Python/libs interfering with ROS2 build
export PATH="/usr/bin:/bin:/opt/ros/humble/bin:${PATH}"
unset CONDA_PREFIX CONDA_DEFAULT_ENV CONDA_SHLVL PYTHONHOME PYTHONPATH
export PYTHON_EXECUTABLE=/usr/bin/python3

echo -e "${YELLOW}Step 1/2: Building core library...${NC}"
cd "${PROJECT_ROOT}"
./build.sh

echo ""
echo -e "${YELLOW}Step 2/2: Building ROS2 package...${NC}"
cd "${ROS2_WS}"
colcon build --packages-select visual2d_infer \
  --cmake-args -DCMAKE_BUILD_TYPE=Release \
  -DPython3_EXECUTABLE=/usr/bin/python3

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  ROS2 build successful!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Source the workspace:"
echo "  source ${ROS2_WS}/install/setup.bash"
echo ""
echo "Run the node:"
echo "  ros2 launch visual2d_infer visual2d_infer.launch.py engine_path:=/path/to/model.engine"
echo ""
echo "Make sure TensorRT libraries are available at runtime:"
echo "  export LD_LIBRARY_PATH=${PROJECT_ROOT}/_LIBRARY/lib:\$LD_LIBRARY_PATH"
