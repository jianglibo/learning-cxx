cmake_minimum_required(VERSION 3.8)

cmake_policy(SET CMP0104 NEW)
cmake_policy(SET CMP0146 OLD)
project(learning-cxx LANGUAGES CXX CUDA)

message(STATUS "*****************CMAKE_MAKE_PROGRAM: ${CMAKE_MAKE_PROGRAM}")

# Check if building on Windows
if(WIN32)
    # Set NVCC flags for Windows build
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --allow-unsupported-compiler")
endif()

if(MSVC)
    # Use dynamic runtime libraries
    # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    # set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    
    # Uncomment the following lines to use static runtime libraries instead
    # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    # set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()


# set(CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES "")

include(cmake/CPM.cmake)

# # We define these as variables so they can be overriden in CI to pull from a PR instead of CCCL `main`
# # In your project, these variables are unncessary and you can just use the values directly
set(CCCL_REPOSITORY "nvidia/cccl" CACHE STRING "GitHub repository to fetch CCCL from")
# set(CCCL_TAG "main" CACHE STRING "Git tag/branch to fetch from CCCL repository")
set(CCCL_TAG "v2.4.0" CACHE STRING "Git tag/branch to fetch from CCCL repository")

# # This will automatically clone CCCL from GitHub and make the exported cmake targets available
CPMAddPackage(
    NAME CCCL
    GITHUB_REPOSITORY ${CCCL_REPOSITORY}
    GIT_TAG ${CCCL_TAG}
)

find_package(Git REQUIRED)

execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "*******GIT_DESCRIBE: ${GIT_DESCRIBE}")
message(STATUS "*******CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")


# message(STATUS "*****ENABLE_ASAN: ${ENABLE_ASAN}******")
# if(ENABLE_ASAN)
#     message(STATUS "******AddressSanitizer is enabled******")
# else()
#     message(STATUS "******AddressSanitizer is disabled******")
# endif()

set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --expt-relaxed-constexpr")

# if not msvc
if(NOT MSVC)
    # Find the path to colormake
    find_program(COLORMAKE_EXECUTABLE colormake)

    if(COLORMAKE_EXECUTABLE)
        set(CMAKE_MAKE_PROGRAM ${COLORMAKE_EXECUTABLE})
    else()
        message(FATAL_ERROR "colormake not found!")
    endif()
    # Enable color output
    set(CMAKE_COLOR_MAKEFILE ON)
endif()


# CMAKE_BUILD_TYPE STREQUAL "Debug" and no 'NO_ASAN' environment variable
if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND ENABLE_ASAN)
# Add AddressSanitizer flags
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        message(STATUS "******AddressSanitizer is enabled******")
        # set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer -g")
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
        # set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${ASAN_FLAGS}")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=always")
    endif()
else()
    message(STATUS "******AddressSanitizer is disabled******")
endif()

# Ensure nvcc uses the host compiler flags for color
if (CMAKE_CUDA_COMPILER)
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler -fdiagnostics-color=always")
endif()


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
     add_definitions(-DDEBUG_BUILD)
else()
     add_definitions(-DRELEASE_BUILD)
endif()



# message(STATUS "******************CMAKE_CUDA_ARCHITECTURES: ${CMAKE_CUDA_ARCHITECTURES}")

# if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
#   set(CMAKE_CUDA_ARCHITECTURES "75;89")
  set(CMAKE_CUDA_ARCHITECTURES native)
# endif()

# Function to print default include paths
function(print_default_include_paths)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -E -v - < /dev/null
            ERROR_VARIABLE COMPILER_OUTPUT
            ERROR_STRIP_TRAILING_WHITESPACE
        )
        string(REGEX MATCHALL "#include <...> search starts here:\n(.*\n)+End of search list." MATCHED ${COMPILER_OUTPUT})
        message(STATUS "Default include paths:\n${MATCHED}")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        file(WRITE "${CMAKE_BINARY_DIR}/dummy.cpp" "")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} /showIncludes /P "${CMAKE_BINARY_DIR}/dummy.cpp"
            ERROR_VARIABLE COMPILER_OUTPUT
            ERROR_STRIP_TRAILING_WHITESPACE
        )
        file(REMOVE "${CMAKE_BINARY_DIR}/dummy.cpp")
        message(STATUS "Default include paths:\n${COMPILER_OUTPUT}")
    else()
        message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

# Call the function to print default include paths
print_default_include_paths()

# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")


# vcpkg_cmake_configure(
#     SOURCE_PATH "${CMAKE_SOURCE_DIR}"
#     OPTIONS
#         -DUSE_OPENMP=ON
# )

# print out current CMAKE_INCLUDE_PATH
message(STATUS "***********CMAKE_INCLUDE_PATH*********: ${CMAKE_INCLUDE_PATH}")
# print out current cmake include path which take effect.
message(STATUS "***********CMAKE_MODULE_PATH*********: ${CMAKE_MODULE_PATH}")
message(STATUS "***********CMAKE_BUILD_PARALLEL_LEVEL*********: $ENV{CMAKE_BUILD_PARALLEL_LEVEL}")


set(CMAKE_CXX_STANDARD 17)

set(learning-cxx_VERSION_MAJOR 0)
set(learning-cxx_VERSION_MINOR 1)
set(learning-cxx_VERSION_PATCH 0)

# Set NVCC flags
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --extended-lambda")

# set(CTEST_USE_LAUNCHERS TRUE)
enable_testing()

# Options to select the BLAS library
option(USE_OPENBLAS "Use OpenBLAS for BLAS and LAPACK" ON)
option(USE_MKL "Use Intel MKL for BLAS and LAPACK" OFF)


add_definitions(-DEIGEN_NO_DEBUG)

add_subdirectory(gtest)
add_subdirectory(bm)
