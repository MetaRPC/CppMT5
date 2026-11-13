cmake_minimum_required(VERSION 3.15)
project(metarpc_cppmt5 VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(BUILD_EXAMPLES "Build examples" OFF)
option(CPPMT5_BUILD_TESTS "Build the offline unit test" ON)

# Dependencies: libprotobuf + grpc++. Resolved via pkg-config, which works consistently on
# both Debian (libgrpc++-dev / libprotobuf-dev ship grpc++.pc, protobuf.pc) and MSYS2/MinGW
# (whose grpc/protobuf CMake config packages are inconsistent). grpc++.pc transitively pulls
# in protobuf, but we request protobuf explicitly too for the generated sources.
#   Debian 12: apt install libprotobuf-dev protobuf-compiler libgrpc++-dev protobuf-compiler-grpc pkg-config
#   MSYS2 UCRT64: pacman -S mingw-w64-ucrt-x86_64-{grpc,protobuf,pkgconf}
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPCPP REQUIRED IMPORTED_TARGET grpc++)
pkg_check_modules(PROTOBUF REQUIRED IMPORTED_TARGET protobuf)
set(CPPMT5_GRPC_TARGET PkgConfig::GRPCPP)
set(CPPMT5_PROTOBUF_TARGET PkgConfig::PROTOBUF)

# ======================================================================================
# gRPC contract → generated stubs (portable across protobuf versions)
# --------------------------------------------------------------------------------------
# The repository ships ONLY the version-independent contract as a FileDescriptorSet in
# proto/mt5_term_api.desc — NOT pre-generated .pb.cc (which are locked to one protobuf
# major version and would not compile against a different one). At configure time we run
# the user's own `protoc` on that descriptor set, so the generated C++/gRPC stubs always
# match the protobuf/grpc the user has installed.
#
# To re-create the descriptor set when the upstream contract changes, build and run the
# `dump_descriptor_set` tool (see examples/tools/dump_descriptor_set.cpp).
# ======================================================================================
find_program(CPPMT5_PROTOC protoc
    DOC "The Protocol Buffers compiler (protobuf-compiler)")
find_program(CPPMT5_GRPC_CPP_PLUGIN grpc_cpp_plugin
    DOC "The gRPC C++ code generator plugin (protobuf-compiler-grpc / grpc)")
if (NOT CPPMT5_PROTOC OR NOT CPPMT5_GRPC_CPP_PLUGIN)
    message(FATAL_ERROR
        "protoc and grpc_cpp_plugin are required to generate the gRPC stubs.\n"
        "  Debian/Ubuntu: apt install protobuf-compiler protobuf-compiler-grpc\n"
        "  MSYS2 UCRT64:  pacman -S mingw-w64-ucrt-x86_64-{protobuf,grpc}\n"
        "  macOS:         brew install protobuf grpc")
endif()

set(CPPMT5_PROTO_DESC ${CMAKE_CURRENT_SOURCE_DIR}/proto/mt5_term_api.desc)
set(CPPMT5_GEN_DIR    ${CMAKE_CURRENT_BINARY_DIR}/generated)
# Re-run CMake configuration (and thus regenerate) whenever the contract changes.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CPPMT5_PROTO_DESC})
file(MAKE_DIRECTORY ${CPPMT5_GEN_DIR})

# The MT5 service/message files (C++ messages + gRPC services).
set(CPPMT5_MT5_PROTOS
    mrpc-mt5-error.proto
    mt5-term-api-account-helper.proto   mt5-term-api-account-information.proto
    mt5-term-api-accounts.proto         mt5-term-api-admin.proto
    mt5-term-api-charts.proto           mt5-term-api-connection.proto
    mt5-term-api-demo-account.proto     mt5-term-api-gui.proto
    mt5-term-api-health-check.proto     mt5-term-api-internal-charts.proto
    mt5-term-api-market-info.proto      mt5-term-api-public-account.proto
    mt5-term-api-quote-history.proto    mt5-term-api-server-timezone.proto
    mt5-term-api-subscriptions.proto    mt5-term-api-trade-functions.proto
    mt5-term-api-trade-stats.proto      mt5-term-api-trading-helper.proto
    mt5-term-api-vnc.proto)

# google/api/annotations is imported by every MT5 file (as a method option); it carries no
# service, so we generate C++ messages only. (Generated in a separate protoc call because
# protoc does not reliably emit a dependency-and-target file alongside its dependents.)
execute_process(
    COMMAND ${CPPMT5_PROTOC} --descriptor_set_in=${CPPMT5_PROTO_DESC}
            --cpp_out=${CPPMT5_GEN_DIR}
            google/api/annotations.proto google/api/http.proto
    RESULT_VARIABLE CPPMT5_GEN_API_RESULT)
execute_process(
    COMMAND ${CPPMT5_PROTOC} --descriptor_set_in=${CPPMT5_PROTO_DESC}
            --cpp_out=${CPPMT5_GEN_DIR}
            --grpc_out=${CPPMT5_GEN_DIR}
            --plugin=protoc-gen-grpc=${CPPMT5_GRPC_CPP_PLUGIN}
            ${CPPMT5_MT5_PROTOS}
    RESULT_VARIABLE CPPMT5_GEN_MT5_RESULT)
if (CPPMT5_GEN_API_RESULT OR CPPMT5_GEN_MT5_RESULT)
    message(FATAL_ERROR "protoc stub generation failed (api=${CPPMT5_GEN_API_RESULT}, mt5=${CPPMT5_GEN_MT5_RESULT})")
endif()

# All generated protobuf + gRPC sources (both *.pb.cc and *.grpc.pb.cc, in the root and in
# google/api/). Well-known types (google/protobuf/*) are NOT generated — libprotobuf ships them.
file(GLOB_RECURSE CPPMT5_PB_SOURCES CONFIGURE_DEPENDS ${CPPMT5_GEN_DIR}/*.pb.cc)

add_library(metarpc_cppmt5 STATIC
    src/mt5_account.cpp     # Layer 1 — low-level (protobuf in/out)
    src/mt5_service.cpp     # Layer 2 — friendly facade
    src/mt5_sugar.cpp       # Layer 3 — high-level conveniences
    ${CPPMT5_PB_SOURCES}
)

target_include_directories(metarpc_cppmt5
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CPPMT5_GEN_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(metarpc_cppmt5 PUBLIC
    ${CPPMT5_GRPC_TARGET}
    ${CPPMT5_PROTOBUF_TARGET}
    Threads::Threads
)

if (NOT MSVC)
    # The ~11 MB of generated code compiles much faster at -O1 and is not on any hot path.
    set_source_files_properties(${CPPMT5_PB_SOURCES} PROPERTIES COMPILE_OPTIONS "-O1;-w")
    target_compile_options(metarpc_cppmt5 PRIVATE -Wall -Wno-deprecated-declarations)
endif()

# --- demos: the examples/demos/ launcher (menu + per-layer demos) --------------------
option(BUILD_DEMOS "Build the examples/demos launcher" ON)
if (BUILD_DEMOS)
    file(GLOB_RECURSE CPPMT5_DEMO_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/demos/*.cpp)
    add_executable(demos ${CPPMT5_DEMO_SOURCES})
    target_include_directories(demos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/examples/demos)
    target_link_libraries(demos PRIVATE metarpc_cppmt5)

    # --- developer tools (examples/tools/) --------------------------------------------
    # protobuf_inspector and dump_descriptor_set walk the generated descriptor pool, so EVERY
    # generated .pb.cc must be linked in (even ones no other code references). Force that with
    # --whole-archive on GNU ld; MSVC uses /WHOLEARCHIVE.
    foreach(_tool protobuf_inspector dump_descriptor_set)
        add_executable(${_tool} ${CMAKE_CURRENT_SOURCE_DIR}/examples/tools/${_tool}.cpp)
        if (MSVC)
            target_link_libraries(${_tool} PRIVATE metarpc_cppmt5)
            set_target_properties(${_tool} PROPERTIES LINK_FLAGS "/WHOLEARCHIVE:metarpc_cppmt5")
        else()
            target_link_libraries(${_tool} PRIVATE
                -Wl,--whole-archive metarpc_cppmt5 -Wl,--no-whole-archive
                ${CPPMT5_GRPC_TARGET} ${CPPMT5_PROTOBUF_TARGET} Threads::Threads)
        endif()
    endforeach()
endif()

# --- quickstart: the smallest standalone example (examples/quickstart.cpp) -----------
if (BUILD_EXAMPLES)
    add_executable(quickstart ${CMAKE_CURRENT_SOURCE_DIR}/examples/quickstart.cpp)
    target_link_libraries(quickstart PRIVATE metarpc_cppmt5)
endif()
