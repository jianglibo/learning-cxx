# include google benchmark
find_package(benchmark)
find_package(fmt CONFIG REQUIRED)


find_package(CUDA REQUIRED)
include_directories(${CUDA_INCLUDE_DIRS})

if(benchmark_FOUND)
  file(GLOB BM_SOURCES *_benchmark.cpp *_benchmark.cu)
  foreach(BM_SOURCE ${BM_SOURCES})
    get_filename_component(OUTPUT ${BM_SOURCE} NAME_WE)
    message("building ${OUTPUT} from ${BM_SOURCE}")

    # for each file, build a binary
    # add_executable(${OUTPUT} ${BM_SOURCE} ../gtest/testutil_host_device.cuh)
    add_executable(${OUTPUT} ${BM_SOURCE})

    include_directories(${CMAKE_SOURCE_DIR})
    # target_include_directories(${OUTPUT}
    #   PRIVATE ../lib/include
    # )

    target_link_libraries(
      ${OUTPUT}
      PRIVATE fmt::fmt-header-only
      PRIVATE benchmark::benchmark
      PRIVATE benchmark::benchmark_main
      PRIVATE CCCL::CCCL
      )
  endforeach()
endif()
