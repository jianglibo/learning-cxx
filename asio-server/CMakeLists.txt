project(DEMO-ASIO-PRJ CXX)

set(APP_NAME asio-server)
set(APP_NAME_ASYNC asio-server-async)
find_package(Boost REQUIRED COMPONENTS asio)

# file(GLOB APP_SOURCE *.cpp)

# message("building ${APP_NAME} from ${APP_SOURCE}")
# add_executable(${APP_NAME} ${APP_SOURCE})
add_executable(${APP_NAME} server-async.cpp)
target_link_libraries(${APP_NAME} PRIVATE Boost::asio)

add_executable(${APP_NAME_ASYNC} server.cpp)
target_link_libraries(${APP_NAME_ASYNC} PRIVATE Boost::asio)
