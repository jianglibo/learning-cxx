project(WORKFLOW-TASTE CXX)

set(APP_NAME workflow-taste)
find_package(Boost REQUIRED COMPONENTS asio)
find_package(workflow CONFIG REQUIRED)

 message(STATUS "************************WORKFLOW_LIB_DIR: ${WORKFLOW_INCLUDE_DIR}***********************")

add_executable(${APP_NAME} src/echo.cpp)

target_include_directories(${APP_NAME}
    PRIVATE workflow)

target_link_libraries(${APP_NAME} 
    PRIVATE Boost::asio
    PRIVATE libworkflow
)
