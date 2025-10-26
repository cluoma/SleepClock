cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

set(PICO_BOARD pico2)
set(PICO_PLATFORM rp2350)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(sleepclock CXX C ASM)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()
# build libraries


# define common sources for pico2maple and pico2maple-w
set(PICO2MAPLE_SRC_COMMON
        src/main.cpp
        src/EddyClock.cpp
        src/EddyClock.h
        src/rv3028.cpp
        src/rv3028.h
        src/button.cpp
        src/button.h
        src/ssd1306.cpp
        src/ssd1306.h
        src/oled_static_data.c
        src/oled_static_data.h
)

add_executable(sleepclock
        ${PICO2MAPLE_SRC_COMMON}
)
#target_compile_options(pico2maple PRIVATE -Wall -Wextra -Wpedantic -Werror)
#target_compile_options(pico2maple PRIVATE -O3)

# Add DEBUG definition if building in Debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(sleepclock PRIVATE DEBUG)
endif()

# enable uart for debugging with debugprobe
pico_enable_stdio_uart(sleepclock 1)

# create uf2
pico_set_uf2_family(${CMAKE_PROJECT_NAME} "rp2350-arm-s")
pico_add_extra_outputs(sleepclock)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(sleepclock PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
)

# link
string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--print-memory-usage")
target_link_libraries(sleepclock PRIVATE
        pico_stdlib
        hardware_i2c
        hardware_dma
)
