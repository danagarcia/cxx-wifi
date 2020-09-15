cmake_minimum_required(VERSION 3.11-3.18)
project(cxx-wifi CXX)

include(FetchContent)

FetchContent_Declare(
    pstreams
    GIT_REPOSITORY https://github.com/jwakely/pstreams.git
    GIT_TAG RELEASE_1_0_1
)

FetchContent_MakeAvailable(pstreams)
add_subdirectory(tests)
add_library(${PROJECT_NAME} src/network.cpp)
target_link_libraries(${PROJECT_NAME} pstreams)