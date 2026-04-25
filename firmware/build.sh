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

SRC_DIR=src/

TARGETS=(pico pico_w pico2 pico2_w)

CMAKE_COMMON="-DCMAKE_BUILD_TYPE=Release -DPICO_FREQ=270000 -DPICO_SDK_PATH=${PICO_SDK_PATH} "

for target in ${TARGETS[@]}; do
    build_dir=build_${target}
    rm -rf ${build_dir}
    mkdir -p ${build_dir}

    echo "=== ${target} ==="
    cmake -B ${build_dir} ${CMAKE_COMMON} -DPICO_BOARD=${target} ${SRC_DIR}
    make -C ${build_dir} -j
done

echo "=== Images ==="
for target in ${TARGETS[@]}; do
    ls build_${target}/*.uf2
done
