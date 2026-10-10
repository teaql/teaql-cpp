get_filename_component(TEAQL_TOOLCHAINS_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../../.tools/toolchains" ABSOLUTE)
set(TEAQL_GCC_ROOT "${TEAQL_TOOLCHAINS_ROOT}/gcc-16.2.0")
set(CMAKE_CXX_COMPILER "${TEAQL_GCC_ROOT}/bin/x86_64-conda-linux-gnu-g++")
# Native packages use the host libc and SQLite, matching the Clang profile.
# The compiler distribution's older sysroot is unsuitable for host SQLite.
set(CMAKE_SYSROOT "/")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,-rpath,${TEAQL_GCC_ROOT}/lib")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT}")
