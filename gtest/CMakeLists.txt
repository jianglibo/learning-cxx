# include catch

set(CMAKE_CXX_STANDARD 17)

set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${ASAN_FLAGS}")

find_package(spdlog CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

find_package(Boost REQUIRED COMPONENTS asio)
find_package(Boost REQUIRED COMPONENTS beast)
find_package(Boost REQUIRED COMPONENTS url)
find_package(Boost REQUIRED COMPONENTS uuid)

find_package(date CONFIG REQUIRED)

find_package(OpenSSL REQUIRED)
find_package(Libssh2 CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost REQUIRED COMPONENTS json)

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
  PRIVATE PRIVATE Boost::json
  PRIVATE  $<IF:$<TARGET_EXISTS:Libssh2::libssh2_shared>,Libssh2::libssh2_shared,Libssh2::libssh2_static>
  )
  add_test(
    NAME ${T_NAME}
    COMMAND ${T_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# -----------------------------------client_async_test.cpp---------------------------------------------
set(T_NAME client_async_test)
add_executable(${T_NAME} client_async_test.cpp)
target_include_directories(${T_NAME} 
  PRIVATE ${CMAKE_SOURCE_DIR}/apps/http_server_async_include
)

target_link_libraries(
  ${T_NAME}
  PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
  PRIVATE Boost::asio
  PRIVATE Boost::beast
  PRIVATE Boost::url
  PRIVATE Boost::uuid
  PRIVATE date::date date::date-tz
  PRIVATE PRIVATE Boost::json
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

target_include_directories(${T_NAME} 
  PRIVATE ${CMAKE_SOURCE_DIR}/apps/http_server_async_include
)
target_link_libraries(
  ${T_NAME}
  PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
  )
  add_test(
    NAME ${T_NAME}
    COMMAND ${T_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

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

# -----------------------------------member_pointer_test.cpp---------------------------------------------
set(T_NAME member_pointer_test)
add_executable(${T_NAME} member_pointer_test.cpp)

target_link_libraries(
  ${T_NAME}
  PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
  )
  add_test(
    NAME ${T_NAME}
    COMMAND ${T_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
# -----------------------------------reference_test.cpp---------------------------------------------
set(T_NAME reference_test)
add_executable(${T_NAME} reference_test.cpp)

target_link_libraries(
  ${T_NAME}
  PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
  )
  add_test(
    NAME ${T_NAME}
    COMMAND ${T_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# -----------------------------------string_all_test.cpp---------------------------------------------
set(T_NAME string_all_test)
add_executable(${T_NAME} string_all_test.cpp)

target_link_libraries(
  ${T_NAME}
  PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
  )
  add_test(
    NAME ${T_NAME}
    COMMAND ${T_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
