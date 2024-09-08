set(APP_NAME asio-client)

find_package(Boost REQUIRED COMPONENTS asio)
message(STATUS "Boost version: ${Boost_VERSION}")
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")


file(GLOB APP_SOURCE *.cpp)

# project(DEMO-ASIO-PRJ CXX)
# message("building ${APP_NAME} from ${APP_SOURCE}")

add_executable(${APP_NAME} ${APP_SOURCE})
# add_executable(${APP_NAME} client.cpp)

target_link_libraries(${APP_NAME} PRIVATE Boost::asio)
