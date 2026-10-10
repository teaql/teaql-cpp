# OpenWrt ath79/nand toolchain for the WNDR4300's MIPS 74Kc.
# Pass -DTEAQL_OPENWRT_TOOLCHAIN_DIR=<toolchain-mips_24kc_... directory>.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR mips)

set(TEAQL_OPENWRT_TOOLCHAIN_DIR "" CACHE PATH "OpenWrt extracted toolchain directory")
if(NOT IS_DIRECTORY "${TEAQL_OPENWRT_TOOLCHAIN_DIR}")
    message(FATAL_ERROR "Set TEAQL_OPENWRT_TOOLCHAIN_DIR to the extracted OpenWrt toolchain directory")
endif()
set(ENV{STAGING_DIR} "${TEAQL_OPENWRT_TOOLCHAIN_DIR}")
set(CMAKE_CXX_COMPILER
    "${TEAQL_OPENWRT_TOOLCHAIN_DIR}/bin/mips-openwrt-linux-musl-g++")
if(NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
    set(CMAKE_CXX_COMPILER_LAUNCHER
        "${CMAKE_COMMAND};-E;env;STAGING_DIR=${TEAQL_OPENWRT_TOOLCHAIN_DIR}")
endif()
set(CMAKE_CXX_FLAGS_INIT "-march=74kc -msoft-float -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-static -static-libgcc -static-libstdc++ -Wl,--gc-sections -Wl,-z,noexecstack")
set(CMAKE_CXX_STANDARD_LIBRARIES "-latomic -Wl,--start-group -lstdc++ -lgcc_eh -Wl,--end-group")
# try_compile configurations must retain the selected extracted toolchain.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES TEAQL_OPENWRT_TOOLCHAIN_DIR)

# QEMU's 24Kc does not implement the DSP instructions emitted for 74Kc.
# The 74Kf model implements them; target compilation still uses soft float.
# Preserve an explicit caller choice, including disabling emulation.
if(NOT DEFINED CMAKE_CROSSCOMPILING_EMULATOR)
    find_program(TEAQL_QEMU_MIPS qemu-mips)
    if(TEAQL_QEMU_MIPS)
        set(CMAKE_CROSSCOMPILING_EMULATOR "${TEAQL_QEMU_MIPS};-cpu;74Kf"
            CACHE STRING "Emulator for MIPS contract checks")
    endif()
endif()
