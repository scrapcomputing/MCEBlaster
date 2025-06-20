#!/bin/bash
set -e

cmake_txt=MDAPio.cmake
echo "# Automatically generated by $(basename ${0})" > ${cmake_txt}

pio_h=MDAPio.h
echo "// Automatically generated by $(basename ${0})" > ${pio_h}

for HSync in PosHSync NegHSync; do
    switch_case_program_cpp=MDASwitchCase_${HSync}_program_cpp
    echo "// Automatically generated by $(basename ${0})" > ${switch_case_program_cpp}

    switch_case_config_cpp=MDASwitchCase_${HSync}_config_cpp
    echo "// Automatically generated by $(basename ${0})" > ${switch_case_config_cpp}

    for i in {5..17}; do
        idx=$(printf "%02d" ${i})
        file="MDA720x350_${HSync}_${idx}.pio"
        delay1=$((${i} - 1 -1))        # -1 for in null, -1 for jmp x--
        offset=$((${i}/2))
        echo 'pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/Pio/MDA720x350_'"${HSync}"'_'"${idx}"'.pio)' >> ${cmake_txt}
        echo "#include \"MDA720x350_${HSync}_${idx}.pio.h\"" >> ${pio_h}
        echo "case ${i}: return &MDA720x350_${HSync}_${idx}_program;" >> ${switch_case_program_cpp}
        echo "case ${i}: return MDA720x350_${HSync}_${idx}_program_get_default_config(Offset);" >> ${switch_case_config_cpp}
        if [ "${HSync}" == "PosHSync" ]; then
            delay2=$((${delay1} - 3))
            cat > ./Pio/${file} <<EOF
;; Copyright (C) 2024 Scrap Computing

.program MDA720x350_${HSync}_${idx}

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
        else
            delay2=$((${delay1} - 2))
            cat > ./Pio/${file} <<EOF
;; Copyright (C) 2025 Scrap Computing

.program MDA720x350_${HSync}_${idx}

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
   jmp pin loop                      ; loop while HSync is 1

   ; HSync is now 0, wait until it becomes 1
wait_hsync_1:
   in null, 32
   push noblock
   jmp pin hsync_is_1                ; break out of loop if HSync is 1
   jmp wait_hsync_1

hsync_is_1:
   wait 1 gpio HSYNC_GPIO [${offset}] ; Wait [${offset}] for beter sampling?
   .wrap                      ; jmp loop
EOF
        fi
    done
done
