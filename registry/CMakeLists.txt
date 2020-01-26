cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(MPLReg VERSION 0.0.1 DESCRIPTION "Registry daemon for MPLogger")
STRING(TOLOWER "${PROJECT_NAME}" PROJECT_FILE_NAME)

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(InstallRequiredSystemLibraries)
include(GNUInstallDirs)
include(CTest)

set(sqlite3_DIR  "${CMAKE_CURRENT_LIST_DIR}/.")

find_package(Git)
find_package(Threads)
find_package(sqlite3 REQUIRED)

find_program(MAKE_EXE NAMES make)
find_program(GIT_EXE NAMES git)

enable_testing()

add_compile_options(
    -Wall -Wpedantic -fexceptions -mcmodel=large
    "$<$<CONFIG:Debug>:-O0;-g3;-ggdb>"
    "$<$<CONFIG:Release>:-O2>"
)

add_compile_definitions(
    FORTIFY_SOURCE=2
    "$<$<CONFIG:Debug>:MALLOC_CHECK_=3;_GLIBCXX_DEBUG>"
)

include_directories(/usr/local/include include)
link_directories(/usr/local/lib)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/lib")

SET(${PROJECT_NAME}_LIB_INSTALL_PATH "${CMAKE_INSTALL_FULL_LIBDIR}/${PROJECT_FILE_NAME}/")
SET(CMAKE_INSTALL_RPATH ${${PROJECT_NAME}_LIB_INSTALL_PATH})
set(${PROJECT_NAME}_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(${PROJECT_NAME}_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(${PROJECT_NAME}_INCLUDE_DIR ${${PROJECT_NAME}_INCLUDE_DIR} PARENT_SCOPE)
set(${PROJECT_NAME}_TEST_DIR ${PROJECT_SOURCE_DIR}/test)

set(${PROJECT_NAME}_REGISTRY_CORE_SOURCES
    ${${PROJECT_NAME}_SOURCE_DIR}/registry_core.cpp
)

protobuf_generate_cpp(_registry_pb_cc _registry_pb_h
                      registry_service.proto)

add_library(${PROJECT_FILE_NAME}_proto STATIC
            ${_registry_pb_h}
            ${_registry_pb_cc}
            registry_service.proto)
include_directories(${CMAKE_BINARY_DIR})
add_custom_target(registry_service_protobuf_TARGET DEPENDS registry_service.proto)
add_dependencies(${PROJECT_FILE_NAME}_proto
                 registry_service_protobuf_TARGET)
target_link_libraries(${PROJECT_FILE_NAME}_proto
                      protobuf::libprotobuf)

set(${PROJECT_NAME}_REGISTRY_CORE_HEADERS
    ${${PROJECT_NAME}_SOURCE_DIR}/registry_core.hpp
    ${_registry_pb_h})

set(${PROJECT_NAME}_MONITOR_SOURCES
    ${${PROJECT_NAME}_SOURCE_DIR}/service_manager.hpp
    ${${PROJECT_NAME}_SOURCE_DIR}/monitor.cpp)

set(${PROJECT_NAME}_REGISTRY_SOURCES
    ${${PROJECT_NAME}_SOURCE_DIR}/registry.cpp)

set(${PROJECT_NAME}_CLIENT_LIB_SOURCES
    ${${PROJECT_NAME}_SOURCE_DIR}/registry_client_lib.cpp)

set(${PROJECT_NAME}_CLIENT_HEADERS
    ${${PROJECT_NAME}_INCLUDE_DIR}/registry_client.hpp
    ${${PROJECT_NAME}_INCLUDE_DIR}/registry_common.hpp
    ${${PROJECT_NAME}_SOURCE_DIR}/registry_lcl.hpp)

add_library(${PROJECT_FILE_NAME}-core-lib SHARED
            ${${PROJECT_NAME}_REGISTRY_CORE_HEADERS}
            ${${PROJECT_NAME}_REGISTRY_CORE_SOURCES})
target_link_libraries(${PROJECT_FILE_NAME}-core-lib
                      glog
                      protobuf::libprotobuf
                      grpc++
                      ${PROJECT_FILE_NAME}_proto
                      ${SQLITE3_LIBRARIES}
                      ${CMAKE_THREAD_LIBS_INIT})

add_library(${PROJECT_FILE_NAME}-client-lib SHARED
            ${${PROJECT_NAME}_CLIENT_HEADERS}
            ${${PROJECT_NAME}_CLIENT_LIB_SOURCES})
target_link_libraries(${PROJECT_FILE_NAME}-client-lib
                      grpc++
                      glog
                      ${SQLITE3_LIBRARIES}
                      ${PROJECT_FILE_NAME}_proto
                      ${CMAKE_THREAD_LIBS_INIT})

add_executable(monitor
               ${${PROJECT_NAME}_MONITOR_SOURCES})
target_link_libraries(monitor
                      glog)

add_executable(${PROJECT_FILE_NAME}
               ${${PROJECT_NAME}_REGISTRY_HEADERS}
               ${${PROJECT_NAME}_REGISTRY_SOURCES})
add_dependencies(${PROJECT_FILE_NAME}
                 protobuf::protoc)
target_link_libraries(${PROJECT_FILE_NAME}
                      glog
                      gflags::gflags
                      ${PROJECT_FILE_NAME}-core-lib
                      ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS ${PROJECT_FILE_NAME}
        DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
        COMPONENT executables)

if (${PROJECT_NAME}_ENABLE_TESTS)

    add_executable(${PROJECT_NAME}_registry_client_tests
                   ${${PROJECT_NAME}_TEST_DIR}/registry_client_tests.cpp)
    target_link_libraries(${PROJECT_NAME}_registry_client_tests
                          ${PROJECT_FILE_NAME}-client-lib
                          ${PROJECT_FILE_NAME}-core-lib
                          gtest_main
                          glog)
    add_test(NAME ${PROJECT_NAME}_registry_client_tests
             COMMAND ${PROJECT_NAME}_registry_client_tests)


    add_executable(${PROJECT_NAME}_registry_core_tests
                   ${${PROJECT_NAME}_TEST_DIR}/registry_core_tests.cpp)
    target_link_libraries(${PROJECT_NAME}_registry_core_tests
                          ${PROJECT_FILE_NAME}-core-lib
                          gtest_main
                          glog)
    add_test(NAME ${PROJECT_NAME}_registry_core_tests
             COMMAND ${PROJECT_NAME}_registry_core_tests)

    add_executable(${PROJECT_NAME}_registry_com_tests
                   ${${PROJECT_NAME}_TEST_DIR}/registry_com_tests.cpp)
    target_link_libraries(${PROJECT_NAME}_registry_com_tests
                          ${PROJECT_FILE_NAME}-core-lib
                          gtest_main
                          glog)
    add_test(NAME ${PROJECT_NAME}_registry_com_tests
             COMMAND ${PROJECT_NAME}_registry_com_tests)

endif()

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Amin")

include(CPack)
