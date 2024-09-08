project(DEMO-PROXY-SERVER CXX)

set(APP_NAME proxy-server)
find_package(Boost REQUIRED COMPONENTS asio)

add_executable(${APP_NAME} proxy-server.cpp ../asio-server/server-async.cpp)
include_directories(${APP_NAME} ${CMAKE_SOURCE_DIR}/asio-server)
target_link_libraries(${APP_NAME} PRIVATE Boost::asio)

# Find OpenSSL libraries
find_package(OpenSSL REQUIRED)
find_package(Libssh2 CONFIG REQUIRED)
find_package(ZLIB REQUIRED)



set(SSH_APP_NAME ssh-proxy-server)

add_executable(${SSH_APP_NAME} ../asio-server/ssh-server.cpp)
include_directories(${SSH_APP_NAME} ${CMAKE_SOURCE_DIR}/asio-server)
target_link_libraries(${SSH_APP_NAME} PRIVATE Boost::asio ZLIB::ZLIB  OpenSSL::SSL OpenSSL::Crypto $<IF:$<TARGET_EXISTS:Libssh2::libssh2_shared>,Libssh2::libssh2_shared,Libssh2::libssh2_static>)
