# include catch
find_package(spdlog CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

find_package(OpenSSL REQUIRED)
find_package(Libssh2 CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

find_package(workflow CONFIG REQUIRED)

# find_path(WORKFLOW_INCLUDE_DIR_1
#   NAMES Workflow.h
#   PATHS ${WORKFLOW_INCLUDE_DIR}/workflow
# )

find_library(WORKFLOW_LIB NAMES workflow PATHS ${WORKFLOW_LIB_DIR})

# message(STATUS "************************WORKFLOW_INCLUDE_DIR: ${WORKFLOW_INCLUDE_DIR_1}***********************")

# include_directories( ${EIGEN3_INCLUDE_DIRS})
# get_target_property(EIGEN3_INCLUDE_DIRS Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
# file(GLOB SOURCES *_test.cpp *_test.cu)
# file(GLOB LIB_SOURCES ../asio-server/*.cpp)
# include_directories(${CMAKE_SOURCE_DIR}/asio-server)

# -----------------------------------server_async_test.cpp---------------------------------------------
set(T_NAME server_async_simple_test)
add_executable(${T_NAME} server_async_simple_test.cpp ${CMAKE_SOURCE_DIR}/asio-server/server-async-simple.cpp)
target_include_directories(${T_NAME} 
  PRIVATE ${CMAKE_SOURCE_DIR}/asio-server
)

target_link_libraries(
  ${T_NAME}
  PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
  PRIVATE  ZLIB::ZLIB 
  PRIVATE OpenSSL::SSL 
  PRIVATE  OpenSSL::Crypto 
  PRIVATE  $<IF:$<TARGET_EXISTS:Libssh2::libssh2_shared>,Libssh2::libssh2_shared,Libssh2::libssh2_static>
  )
  add_test(
    NAME ${T_NAME}
    COMMAND ${T_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# -----------------------------------shell_test.cpp---------------------------------------------
set(T_NAME shell_test)
add_executable(${T_NAME} shell_test.cpp)

target_link_libraries(
  ${T_NAME}
  PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
  )
  add_test(
    NAME ${T_NAME}
    COMMAND ${T_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# -----------------------------------wf_test.cpp---------------------------------------------
set(T_NAME wf_test)
add_executable(${T_NAME} wf_test.cpp)

target_include_directories(${T_NAME}
    PRIVATE ${WORKFLOW_INCLUDE_DIR}
    )

target_link_libraries(
  ${T_NAME}
  PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
  PRIVATE ${WORKFLOW_LIB}
  )
  add_test(
    NAME ${T_NAME}
    COMMAND ${T_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# -----------------------------------wf_client.cpp---------------------------------------------
set(T_NAME wf_client)
add_executable(${T_NAME} wf_client.cpp)

target_include_directories(${T_NAME}
    PRIVATE ${WORKFLOW_INCLUDE_DIR}
    )

target_link_libraries(${T_NAME}
  PRIVATE ${WORKFLOW_LIB}
  )

# -----------------------------------polymorphisam_test.cpp---------------------------------------------
set(T_NAME polymorphisam_test)
add_executable(${T_NAME} polymorphisam_test.cpp)

target_link_libraries(
  ${T_NAME}
  PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
  )
  add_test(
    NAME ${T_NAME}
    COMMAND ${T_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})