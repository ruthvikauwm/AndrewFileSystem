cmake_minimum_required(VERSION 3.19)

project (uwafs
         DESCRIPTION "A FUSE-based fault injection filesystem.")

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

set(CMAKE_CXX_STANDARD 14)

find_package(Threads)

set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

get_filename_component(uwAFS_proto "./uwAFS.proto" ABSOLUTE)
get_filename_component(uwAFS_proto_path "${uwAFS_proto}" PATH)

set(uwAFS_proto_sources "${CMAKE_CURRENT_BINARY_DIR}/uwAFS.pb.cc")
set(uwAFS_proto_headers "${CMAKE_CURRENT_BINARY_DIR}/uwAFS.pb.h")
set(uwAFS_grpc_sources "${CMAKE_CURRENT_BINARY_DIR}/uwAFS.grpc.pb.cc")
set(uwAFS_grpc_headers "${CMAKE_CURRENT_BINARY_DIR}/uwAFS.grpc.pb.h")
add_custom_command(
        OUTPUT "${uwAFS_proto_sources}" "${uwAFS_proto_headers}" "${uwAFS_grpc_sources}" "${uwAFS_grpc_headers}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}" --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${uwAFS_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${uwAFS_proto}"
        DEPENDS "${uwAFS_proto}")

add_library(uwAFS_grpc_proto
        ${uwAFS_proto_sources}
        ${uwAFS_proto_headers}
        ${uwAFS_grpc_sources}
        ${uwAFS_grpc_headers})
target_link_libraries(uwAFS_grpc_proto
        ${_PROTOBUF_LIBPROTOBUF}
        ${_REFLECTION}
        ${_GRPC_GRPCPP})

file(COPY server.config
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
   

add_subdirectory(uwAFS_client)
add_subdirectory(uwAFS_server)