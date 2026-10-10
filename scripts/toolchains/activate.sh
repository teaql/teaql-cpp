# Source this file from bash: source scripts/toolchains/activate.sh
teaql_toolchain_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)/.tools/toolchains"
export PATH="${teaql_toolchain_root}/cmake-4.4.4/bin:${teaql_toolchain_root}/ninja-1.13.2:${teaql_toolchain_root}/llvm-23.1.3/bin:${teaql_toolchain_root}/gcc-16.2.0/bin:${PATH}"
export CC="${teaql_toolchain_root}/llvm-23.1.3/bin/clang"
export CXX="${teaql_toolchain_root}/llvm-23.1.3/bin/clang++"
export STAGING_DIR="${teaql_toolchain_root}/openwrt-25.12.5/toolchain-mips_24kc_gcc-14.3.0_musl"
export LD_LIBRARY_PATH="${teaql_toolchain_root}/gcc-16.2.0/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
unset teaql_toolchain_root
