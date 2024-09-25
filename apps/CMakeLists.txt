project(DEMO-PROXY-SERVER CXX)

set(APP_NAME proxy-server)
find_package(Boost REQUIRED COMPONENTS asio)
find_package(Boost REQUIRED COMPONENTS beast)
find_package(Boost REQUIRED COMPONENTS url)
find_package(Boost REQUIRED COMPONENTS uuid)

find_package(date CONFIG REQUIRED)
# Find OpenSSL libraries
find_package(OpenSSL REQUIRED)
find_package(Libssh2 CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost REQUIRED COMPONENTS json)


add_executable(${APP_NAME} proxy-server.cpp ../asio-server/server-async-simple.cpp)
include_directories(${APP_NAME} ${CMAKE_SOURCE_DIR}/asio-server)
target_link_libraries(${APP_NAME} 
    PRIVATE Boost::asio
    PRIVATE Boost::beast
    )


set(BOOST_SERVER http_server_async)
add_executable(${BOOST_SERVER} http_server_async.cpp)
target_include_directories(${BOOST_SERVER} 
    PRIVATE http_server_async_include
    )
target_link_libraries(${BOOST_SERVER} 
    PRIVATE Boost::asio
    PRIVATE Boost::beast
    PRIVATE Boost::url
    PRIVATE Boost::json
    PRIVATE Boost::uuid
    PRIVATE date::date date::date-tz
    PRIVATE ZLIB::ZLIB 
    PRIVATE OpenSSL::SSL 
    PRIVATE  OpenSSL::Crypto 
    )

set(BOOST_RELAY boost_relay)
add_executable(${BOOST_RELAY} boost_relay.cpp)
target_link_libraries(${BOOST_RELAY} 
    PRIVATE Boost::asio
    PRIVATE Boost::beast
    )







set(SSH_APP_NAME ssh-proxy-server)

add_executable(${SSH_APP_NAME} ../asio-server/ssh-server.cpp)
include_directories(${SSH_APP_NAME} ${CMAKE_SOURCE_DIR}/asio-server)
target_link_libraries(${SSH_APP_NAME}
 PRIVATE Boost::asio 
 PRIVATE  ZLIB::ZLIB 
 PRIVATE OpenSSL::SSL 
 PRIVATE  OpenSSL::Crypto 
 PRIVATE  $<IF:$<TARGET_EXISTS:Libssh2::libssh2_shared>,Libssh2::libssh2_shared,Libssh2::libssh2_static>
 )
