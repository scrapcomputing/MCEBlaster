#!/bin/bash

cmake_txt=CGAPio.cmake
echo "# Automatically generated by $(basename ${0})" > ${cmake_txt}

pio_h=CGAPio.h
echo "// Automatically generated by $(basename ${0})" > ${pio_h}

switch_case_program_cpp=CGASwitchCase_program_cpp
echo "// Automatically generated by $(basename ${0})" > ${switch_case_program_cpp}

switch_case_config_cpp=CGASwitchCase_config_cpp
echo "// Automatically generated by $(basename ${0})" > ${switch_case_config_cpp}

for i in {4..16}; do
    idx=$(printf "%02d" ${i})
    file="CGA640x200_${idx}.pio"
    delay1=$((${i} - 1))
    delay2=$((${delay1} - 3))
    offset=$((${i}/2))
    echo 'pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/Pio/CGA640x200_'"${idx}"'.pio)' >> ${cmake_txt}
    echo "#include \"CGA640x200_${idx}.pio.h\"" >> ${pio_h}
    echo "case ${i}: return &CGA640x200_${idx}_program;" >> ${switch_case_program_cpp}
    echo "case ${i}: return CGA640x200_${idx}_program_get_default_config(Offset);" >> ${switch_case_config_cpp}
cat > ./Pio/${file} <<EOF
;; Copyright (C) 2024 Scrap Computing
.program CGA640x200_${idx}
.define TTL_PIN_CNT 8
.define HSYNC_GPIO 7

; Pixel clock 640 mode: 14.318MHz (315/22):  69.84ns/pixel 18.857 instrs (3.704ns/instr)
; Pixel clock 320 mode:  7.159MHz (315/44): 140ns/pixel

entry:

.wrap_target
loop:
   in pins, TTL_PIN_CNT [${delay1}]
   in pins, TTL_PIN_CNT [${delay1}]
   in pins, TTL_PIN_CNT [${delay1}]
   in pins, TTL_PIN_CNT [${delay2}]   ; ISR = RRGGBBHV(#3),RRGGBBHV(#2),RRGGBBHV(#1),RRGGBBHV(#0)
   push noblock               ; FIFO = ISR (#3,#2,#1,#0)
   jmp pin wait_hsync
   jmp loop

wait_hsync:
   in null, 32
   push noblock
   jmp pin wait_hsync
   wait 0 gpio HSYNC_GPIO [${offset}] ; Wait [${offset}] for better sampling?
   .wrap                      ; jmp loop
EOF
done