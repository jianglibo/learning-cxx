project(CUSTOM_PAM CXX)

set(APP_NAME custom_pam)


include(../cmake/Modules/FindPAM.cmake)

# Find and link PAM
# find_package(PAM REQUIRED)

include_directories(${PAM_INCLUDE_DIRS})

add_library(${APP_NAME} SHARED cpam_auth.cpp)
include_directories(${APP_NAME} ./)
target_link_libraries(${APP_NAME} PRIVATE ${PAM_LIBRARIES})

