# include catch
find_package(spdlog CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)


include_directories( ${EIGEN3_INCLUDE_DIRS} )
get_target_property(EIGEN3_INCLUDE_DIRS Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)

file(GLOB SOURCES *_test.cpp *_test.cu)

file(GLOB LIB_SOURCES ../asio-server/*.cpp)
include_directories(${CMAKE_SOURCE_DIR}/asio-server)

add_definitions(-DTEST_ENV)

foreach(SOURCE ${SOURCES})
  get_filename_component(OUTPUT ${SOURCE} NAME_WE)

  project(${OUTPUT}-PRJ CXX)
  message("building ${OUTPUT} from ${SOURCE}")

  add_executable(${OUTPUT} ${SOURCE} ${LIB_SOURCES})
  target_link_libraries(
    ${OUTPUT}
  )

  target_link_libraries(
    ${OUTPUT}
    PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
    )
    add_test(
      NAME ${OUTPUT}
      COMMAND ${OUTPUT}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endforeach()