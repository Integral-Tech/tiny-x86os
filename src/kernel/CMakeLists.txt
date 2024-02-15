# SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

project(kernel LANGUAGES C)

set(LIBS_FLAGS "-L ${CMAKE_BINARY_DIR}/src/applib/")
set(CMAKE_EXE_LINKER_FLAGS "-m elf_i386 -T ${PROJECT_SOURCE_DIR}/kernel.lds ${LIBS_FLAGS}")
set(CMAKE_C_LINK_EXECUTABLE "${LINKER_TOOL} <OBJECTS> ${CMAKE_EXE_LINKER_FLAGS} -o ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf")

file(GLOB_RECURSE C_LIST "*.c" "*.h" "*.S")
add_executable(${PROJECT_NAME} init/start.S ${C_LIST})

add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${OBJCOPY_TOOL} -S ${PROJECT_NAME}.elf ${CMAKE_SOURCE_DIR}/images/${PROJECT_NAME}.elf
                   COMMAND ${OBJDUMP_TOOL} -x -d -S -m i386 ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf > ${PROJECT_NAME}_dis.txt
                   COMMAND ${READELF_TOOL} -a ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf > ${PROJECT_NAME}_elf.txt
)
