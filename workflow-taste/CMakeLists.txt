project(WORKFLOW-TASTE CXX)

set(APP_NAME workflow-taste)
find_package(Boost REQUIRED COMPONENTS asio)
find_package(workflow CONFIG REQUIRED )

add_executable(${APP_NAME} src/echo.cpp)
target_include_directories(${APP_NAME} PRIVATE ${WORKFLOW_INCLUDE_DIR})
target_link_libraries(${APP_NAME} 
    PRIVATE Boost::asio
    PRIVATE ${WORKFLOW_LIB_DIR}/libworkflow.so
)
