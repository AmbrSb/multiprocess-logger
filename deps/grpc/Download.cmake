set(workdir "${CMAKE_BINARY_DIR}/grpc-download")

configure_file("${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.in"
               "${workdir}/CMakeLists.txt" @ONLY)

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

add_subdirectory("${CMAKE_BINARY_DIR}/grpc-src"
                 "${CMAKE_BINARY_DIR}/grpc-build" EXCLUDE_FROM_ALL)

include_directories("${CMAKE_BINARY_DIR}/grpc-build/third_party/protobuf/src")
include_directories("${CMAKE_BINARY_DIR}/grpc-src/include")
