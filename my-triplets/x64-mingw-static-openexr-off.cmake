set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_ENV_PASSTHROUGH PATH)
set(WITH_OPENEXR OFF)

set(VCPKG_CMAKE_SYSTEM_NAME MinGW)
set(CMAKE_SYSTEM_NAME Windows)

set(CMAKE_C_COMPILER "D:/msys64/mingw64/bin/x86_64-w64-mingw32-gcc.exe")
set(CMAKE_CXX_COMPILER "D:/msys64/mingw64/bin/x86_64-w64-mingw32-g++.exe")
set(CMAKE_CUDA_HOST_COMPILER "D:/msys64/mingw64/bin/x86_64-w64-mingw32-g++.exe")
set(CMAKE_RC_COMPILER "D:/msys64/mingw64/bin/x86_64-w64-mingw32-windres.exe")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m64")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m64")

message(WARNING "**********INITIAL ADDITIONAL BUILD FLAGS**********: ${ADDITIONAL_BUILD_FLAGS}")
list(APPEND ADDITIONAL_BUILD_FLAGS
  "-DCUDA_ARCH_BIN=8.9 7.5"
)