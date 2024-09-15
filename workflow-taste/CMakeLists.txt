project(WORKFLOW-TASTE CXX)

set(APP_NAME workflow-taste)
find_package(Boost REQUIRED COMPONENTS asio)
find_package(workflow CONFIG REQUIRED)

find_path(WORKFLOW_INCLUDE_DIR_1
  NAMES Workflow.h
  PATHS ${WORKFLOW_INCLUDE_DIR}/workflow
)

 message(STATUS "************************WORKFLOW_INCLUDE_DIR: ${WORKFLOW_INCLUDE_DIR_1}***********************")

add_executable(${APP_NAME} src/echo.cpp)

target_include_directories(${APP_NAME}
    PRIVATE ${WORKFLOW_INCLUDE_DIR_1}
    )

find_library(WORKFLOW_LIB NAMES workflow PATHS ${WORKFLOW_LIB_DIR})

target_link_libraries(${APP_NAME} 
    PRIVATE Boost::asio
    PRIVATE ${WORKFLOW_LIB}
)
