set(workdir "${CMAKE_BINARY_DIR}/protobuf-download")

configure_file("${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.in"
               "${workdir}/CMakeLists.txt")

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                RESULT_VARIABLE error
                WORKING_DIRECTORY "${workdir}")
if(error)
  message(FATAL_ERROR "CMake step for ${PROJECT_NAME} failed: ${error}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build .
                RESULT_VARIABLE error
                WORKING_DIRECTORY "${workdir}")
if(error)
  message(FATAL_ERROR "Build step for ${PROJECT_NAME} failed: ${error}")
endif()

set(protobuf_BUILD_TESTS OFF CACHE BOOL "")
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(protobuf_WITH_ZLIB OFF CACHE BOOL "")

add_subdirectory("${CMAKE_BINARY_DIR}/protobuf-src/cmake"
                 "${CMAKE_BINARY_DIR}/protobuf-build" EXCLUDE_FROM_ALL)
get_property(Protobuf_INCLUDE_DIRS TARGET protobuf::libprotobuf
                                   PROPERTY INCLUDE_DIRECTORIES)
