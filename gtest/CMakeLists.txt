#
# unit tests:
#

# include catch
find_package(spdlog CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

find_package(OpenMP REQUIRED)

find_package(CUDA REQUIRED)
include_directories(${CUDA_INCLUDE_DIRS})
message(STATUS "********************CUDA_INCLUDE_DIRS: ${CUDA_INCLUDE_DIRS}")
message(STATUS "********************CCCL::CCCL: ${CCCL}")

find_package(Eigen3 CONFIG REQUIRED)
include_directories( ${EIGEN3_INCLUDE_DIRS} )
get_target_property(EIGEN3_INCLUDE_DIRS Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Eigen3_DIR: ${EIGEN3_INCLUDE_DIRS}")

# find_package(OpenCV REQUIRED)
# set(OpenCV_DIR /opt/opencv/lib/cmake/opencv4)
# find_package(OpenCV REQUIRED
#   COMPONENTS core imgproc highgui cudaarithm cudaimgproc cudafilters)

# --gtest_color=yes
option(GTEST_COLOR "Enable colored output for gtest" ON)

file(GLOB SOURCES *_test.cpp *_test.cu)


# add_definitions(-DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_OMP) # For OpenMP
# list(FILTER LIB_SOURCES EXCLUDE REGEX ".*main.cu")

# add_library(pch INTERFACE)
# target_precompile_headers(pch INTERFACE pch.h)

add_definitions(-DTEST_ENV)

foreach(SOURCE ${SOURCES})
  get_filename_component(OUTPUT ${SOURCE} NAME_WE)

  set(MY_VARIABLE "Hello World")

  project(${OUTPUT}-PRJ CXX CUDA)
  message("building ${OUTPUT} from ${SOURCE}")

  # for each file, build a binary
  # add_executable(${OUTPUT} ${SOURCE} ${LIB_SOURCES})
  # if output endswith .cu
  if(${SOURCE} MATCHES ".*cu$")
    # add_executable(${OUTPUT} ${SOURCE} testutil_host_device.cuh testutil_device.cuh)
    add_executable(${OUTPUT} ${SOURCE})
  else()
    # add_executable(${OUTPUT} ${SOURCE} testutil_host_device.cuh)
    add_executable(${OUTPUT} ${SOURCE})
  endif()

  if(OpenMP_CXX_FOUND)
      target_compile_definitions(${OUTPUT} PRIVATE THRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_OMP)
      target_compile_options(${OUTPUT} PRIVATE ${OpenMP_CXX_FLAGS})
      target_link_libraries(${OUTPUT} PRIVATE OpenMP::OpenMP_CXX)
  endif()

  target_link_libraries(
    ${OUTPUT}
  )
  # message(STATUS "***********${LIB_NAME}***********")
  # target_include_directories(${OUTPUT})

  target_link_libraries(
    ${OUTPUT}
    PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
    )
  # add as a test
    add_test(
      NAME ${OUTPUT}
      COMMAND ${OUTPUT}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endforeach()