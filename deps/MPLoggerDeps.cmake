# if(${PROJECT_NAME}_USE_GFLAGS)
#     include(deps/gflags/Download.cmake)
# else()
#     message("GFLAGS is disabled")
# endif()

if(${PROJECT_NAME}_USE_GLOG)
    include(deps/glog/Download.cmake)
else()
    message("GLOG is disabled")
endif()

if(${PROJECT_NAME}_USE_GSL)
    include(deps/gsl/Download.cmake)
else()
    message("GSL is disabled")
endif()

if(${PROJECT_NAME}_USE_GOOGLETEST)
    include(deps/googletest/Download.cmake)
endif()

# find_package(Protobuf REQUIRED)
# find_package(GRPC CONFIG REQUIRED)

# if(${PROJECT_NAME}_USE_PROTOBUFS)
#     include(deps/protobufs/Download.cmake)
# endif()

# if(${PROJECT_NAME}_USE_CARES)
#     include(deps/cares/Download.cmake)
# endif()

if(${PROJECT_NAME}_USE_GRPC)
    include(deps/grpc/Download.cmake)
endif()
