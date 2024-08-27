set(APP_NAME asio-client)

find_package(Boost REQUIRED COMPONENTS asio)


file(GLOB APP_SOURCE *.cpp)

project(DEMO-ASIO-PRJ CXX)
message("building ${APP_NAME} from ${APP_SOURCE}")

add_executable(${APP_NAME} ${APP_SOURCE})

target_link_libraries(${APP_NAME} PRIVATE Boost::asio)
