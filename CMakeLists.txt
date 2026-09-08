cmake_minimum_required(VERSION 3.15)
project(metarpc_cppmt5 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(metarpc_cppmt5 STATIC
    src/client.cpp
)

target_include_directories(metarpc_cppmt5 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if (BUILD_EXAMPLES)
    add_executable(quickstart examples/quickstart.cpp)
    target_link_libraries(quickstart PRIVATE metarpc_cppmt5)
endif()

enable_testing()
add_executable(test_client tests/test_client.cpp)
target_link_libraries(test_client PRIVATE metarpc_cppmt5)
add_test(NAME test_client COMMAND test_client)