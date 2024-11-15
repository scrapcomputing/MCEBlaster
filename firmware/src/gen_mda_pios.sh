#!/bin/bash

cmake_txt=MDAPio.cmake
echo "# Automatically generated by $(basename ${0})" > ${cmake_txt}

pio_h=MDAPio.h
echo "// Automatically generated by $(basename ${0})" > ${pio_h}

switch_case_program_cpp=MDASwitchCase_program_cpp
echo "// Automatically generated by $(basename ${0})" > ${switch_case_program_cpp}

switch_case_config_cpp=MDASwitchCase_config_cpp
echo "// Automatically generated by $(basename ${0})" > ${switch_case_config_cpp}

for i in {5..17}; do
    idx=$(printf "%02d" ${i})
    file="MDA720x350_${idx}.pio"
    delay1=$((${i} - 1 -1))        # -1 for in null, -1 for jmp x--
    delay2=$((${delay1} - 3))
    offset=$((${i}/2))
    echo 'pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/Pio/MDA720x350_'"${idx}"'.pio)' >> ${cmake_txt}
    echo "#include \"MDA720x350_${idx}.pio.h\"" >> ${pio_h}
    echo "case ${i}: return &MDA720x350_${idx}_program;" >> ${switch_case_program_cpp}
    echo "case ${i}: return MDA720x350_${idx}_program_get_default_config(Offset);" >> ${switch_case_config_cpp}
cat > ./Pio/${file} <<EOF
;; Copyright (C) 2024 Scrap Computing

.program MDA720x350_${idx}

.define TTL_PIN_CNT 4
.define HSYNC_GPIO 7

; Pixel clock: 16.257MHz: 61.512ns/pixel 3.704ns/instr @ 270MHz , so we need 16.66 instrs / pixel

entry:

.wrap_target
loop:
   set x, 6
   bundle:
     in pins, TTL_PIN_CNT [${delay1}]  ; ISR = XXIV(#0)
     jmp x--, bundle
   in pins, TTL_PIN_CNT [${delay2}]  ; ISR = XXIV(#7)XXIV(#6)...XXIV(#0)
   push noblock                      ; FIFO = ISR (#7,#6,#5,#4,#3,#2,#1,#0)
   jmp pin wait_hsync
   jmp loop

wait_hsync:
   in null, 32
   push noblock
   jmp pin wait_hsync
   wait 0 gpio HSYNC_GPIO [${offset}] ; Wait [${offset}] for beter sampling?
   .wrap                      ; jmp loop
EOF
done
