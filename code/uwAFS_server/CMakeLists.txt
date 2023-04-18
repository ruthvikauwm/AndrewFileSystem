include_directories("${CMAKE_CURRENT_BINARY_DIR}")

foreach(_target server)
    add_executable(${_target} "${_target}.cc")
    target_link_libraries(${_target}
            uwAFS_grpc_proto
            ${_PROTOBUF_LIBPROTOBUF}
            ${_REFLECTION}
            ${_GRPC_GRPCPP})


endforeach()