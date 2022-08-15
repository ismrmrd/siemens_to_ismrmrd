#!/bin/bash

set -euo pipefail

mkdir -p build
cd build
cmake -G Ninja -DBUILD_DYNAMIC=ON -DCMAKE_PREFIX_PATH=${PREFIX} -DCMAKE_INSTALL_PREFIX=${PREFIX} ../
ninja
ninja install