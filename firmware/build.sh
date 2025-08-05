#!/bin/bash
#
# Copyright (C) 2025 Scrap Computing
#
# A helper script for building the release firmwares for both Pico 1 and Pico 2
#

set -e

if [ $# -ne 1 ]; then
    echo "USAGE: $(basename ${0}) <path/to/pico-sdk>"
    exit 1
fi

PICO_SDK_PATH=${1}

echo "PICO_SDK_PATH=${PICO_SDK_PATH}"

BUILD_PICO1=build_pico1
BUILD_PICO2=build_pico2
for dir in "${BUILD_PICO1}" "${BUILD_PICO2}"; do
    rm -rf ${dir}
    mkdir -p ${dir}
done

SRC_DIR=src/
CMAKE_COMMON="-DCMAKE_BUILD_TYPE=Release -DPICO_FREQ=270000 -DPICO_SDK_PATH=${PICO_SDK_PATH} "

echo "=== Pico 1 ==="
cmake -B ${BUILD_PICO1} ${CMAKE_COMMON} -DPICO_BOARD=pico ${SRC_DIR}
make -C ${BUILD_PICO1} -j

echo "=== Pico 2 ==="
cmake -B ${BUILD_PICO2} ${CMAKE_COMMON} -DPICO_BOARD=pico2 ${SRC_DIR}
make -C ${BUILD_PICO2} -j

echo "Done!"
for dir in "${BUILD_PICO1}" "${BUILD_PICO2}"; do
    ls ${dir}/*.uf2
done
