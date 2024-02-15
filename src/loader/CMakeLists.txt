# SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

project(loader LANGUAGES C)

set(CMAKE_EXE_LINKER_FLAGS "-m elf_i386 -Ttext=0x8000")
set(CMAKE_C_LINK_EXECUTABLE "${LINKER_TOOL} <OBJECTS> ${CMAKE_EXE_LINKER_FLAGS} -o ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf")

file(GLOB C_LIST "*.c" "*.h")
add_executable(${PROJECT_NAME} start.S ${C_LIST})

add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${OBJCOPY_TOOL} -O binary ${PROJECT_NAME}.elf ${CMAKE_SOURCE_DIR}/images/${PROJECT_NAME}.bin
                   COMMAND ${OBJDUMP_TOOL} -x -d -S -m i8086 ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf > ${PROJECT_NAME}_dis.txt
                   COMMAND ${READELF_TOOL} -a ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf > ${PROJECT_NAME}_elf.txt
)
