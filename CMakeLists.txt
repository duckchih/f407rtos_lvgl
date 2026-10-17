cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME f407rtos_lvgl)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# ============================================================
# FreeRTOS 内核（编译成独立静态库）
# ============================================================
set(FREERTOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/FreeRTOS)

add_library(freertos_kernel STATIC
    ${FREERTOS_DIR}/Source/tasks.c
    ${FREERTOS_DIR}/Source/queue.c
    ${FREERTOS_DIR}/Source/list.c
    ${FREERTOS_DIR}/Source/timers.c
    ${FREERTOS_DIR}/Source/event_groups.c
    ${FREERTOS_DIR}/Source/stream_buffer.c
    ${FREERTOS_DIR}/Source/portable/GCC/ARM_CM4F/port.c
    ${FREERTOS_DIR}/Source/portable/MemMang/heap_4.c
)

# FreeRTOS 源码里要 include "stm32f4xx.h" 和 "FreeRTOSConfig.h"
target_include_directories(freertos_kernel PUBLIC
    ${FREERTOS_DIR}/Source/include
    ${FREERTOS_DIR}/Source/portable/GCC/ARM_CM4F
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
)


# FreeRTOS 依赖 HAL 提供的 CMSIS 头文件
target_link_libraries(freertos_kernel PUBLIC stm32cubemx)


# ============================================================
# LVGL 图形库
# ============================================================
set(LVGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/LVGL)

# 递归收集 LVGL src 下的所有 .c 文件
file(GLOB_RECURSE LVGL_SOURCES ${LVGL_DIR}/src/*.c)

add_library(lvgl STATIC ${LVGL_SOURCES})

target_include_directories(lvgl PUBLIC
    ${LVGL_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
)

target_link_libraries(lvgl PUBLIC stm32cubemx freertos_kernel)


# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    BSP/LCD/lcd.c
    BSP/LVGL/lv_port_disp.c
    App/Src/app.c
    App/Src/ui.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    BSP/LCD
    BSP/LVGL
    App/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    freertos_kernel      # <-- 加这一行
    lvgl
)
