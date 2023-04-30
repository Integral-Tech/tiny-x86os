cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_VERBOSE_MAKEFILE on)

if (CMAKE_HOST_WIN32 OR CMAKE_HOST_APPLE)
    set(TOOL_PREFIX "x86_64-elf-")
elseif (CMAKE_HOST_LINUX)
    set(TOOL_PREFIX "x86_64-linux-gnu-")
endif ()

set(CMAKE_C_COMPILER "${TOOL_PREFIX}gcc") 
set(CMAKE_C_FLAGS "-g -c -O0 -Wall -Wextra -march=i486 -m32 -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables")

set(CMAKE_ASM_COMPILER "${TOOL_PREFIX}gcc")
set(CMAKE_ASM_FLAGS "-m32 -g")
set(CMAKE_ASM_SOURCE_FILE_EXTENSIONS "asm")

set(LINKER_TOOL "ld")

set(OBJCOPY_TOOL "objcopy")
set(OBJDUMP_TOOL "objdump")
set(READELF_TOOL "readelf")

project(os LANGUAGES C)  
enable_language(ASM)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/kernel/include
    ${PROJECT_SOURCE_DIR}/newlib/i686-elf/include
)

add_subdirectory(./src/boot)
add_subdirectory(./src/loader)
add_subdirectory(./src/kernel)
add_subdirectory(./src/applib)
add_subdirectory(./src/shell)
add_subdirectory(./src/apps/uname)

add_dependencies(kernel app)
add_dependencies(shell app)
