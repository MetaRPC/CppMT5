cmake_minimum_required(VERSION 3.15)
project(metarpc_cppmt5 VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(BUILD_EXAMPLES "Build examples" OFF)
option(CPPMT5_BUILD_TESTS "Build the offline unit test" ON)

# Dependencies: libprotobuf 3.21.x (generated code is protoc 3.21.6) and grpc++ >= 1.4x.
# Debian 12: apt install libprotobuf-dev protobuf-compiler libgrpc++-dev protobuf-compiler-grpc
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG QUIET)
if (gRPC_FOUND)
    set(CPPMT5_GRPC_TARGET gRPC::grpc++)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GRPCPP REQUIRED IMPORTED_TARGET grpc++)
    set(CPPMT5_GRPC_TARGET PkgConfig::GRPCPP)
endif()

# All generated protobuf/gRPC sources (they cross-include each other, so compile all of them),
# plus package/google/api/{annotations,http}.pb.cc which every mt5 header includes.
file(GLOB_RECURSE CPPMT5_PB_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/package/*.pb.cc)

add_library(metarpc_cppmt5 STATIC
    src/client.cpp
    ${CPPMT5_PB_SOURCES}
)

target_include_directories(metarpc_cppmt5
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/package>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(metarpc_cppmt5 PUBLIC
    ${CPPMT5_GRPC_TARGET}
    protobuf::libprotobuf
    Threads::Threads
)

if (NOT MSVC)
    # The 11 MB of generated code compiles much faster at -O1 and is not on any hot path.
    set_source_files_properties(${CPPMT5_PB_SOURCES} PROPERTIES COMPILE_OPTIONS "-O1;-w")
    target_compile_options(metarpc_cppmt5 PRIVATE -Wall -Wno-deprecated-declarations)
endif()

if (BUILD_EXAMPLES)
    add_executable(quickstart examples/quickstart.cpp)
    target_link_libraries(quickstart PRIVATE metarpc_cppmt5)
endif()

if (CPPMT5_BUILD_TESTS)
    enable_testing()
    add_executable(test_client tests/test_client.cpp)
    target_link_libraries(test_client PRIVATE metarpc_cppmt5)
    add_test(NAME test_client COMMAND test_client)
endif()
