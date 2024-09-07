project(DEMO-PROXY-SERVER CXX)

set(APP_NAME proxy-server)
find_package(Boost REQUIRED COMPONENTS asio)

add_executable(${APP_NAME} proxy-server.cpp ../asio-server/server-async.cpp)
include_directories(${APP_NAME} ${CMAKE_SOURCE_DIR}/asio-server)
target_link_libraries(${APP_NAME} PRIVATE Boost::asio)
