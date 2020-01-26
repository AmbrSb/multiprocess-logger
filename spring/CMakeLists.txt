cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(spring VERSION 0.0.1 DESCRIPTION "Spring for IPC")
STRING(TOLOWER "${PROJECT_NAME}" PROJECT_FILE_NAME)

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(InstallRequiredSystemLibraries)
include(GNUInstallDirs)
include(CTest)

find_package(Git)
find_package(Threads)
find_library(LIBRT rt)                                                                                                                                                                                                                                                                                        
    if(NOT LIBRT)
        message(FATAL_ERROR "Cannot find librt")
endif()

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

include_directories(/usr/local/include
                    /usr/include
                    include
                    ${MPLReg_INCLUDE_DIR}
                    ${mpmc_ring_INCLUDE_DIR})

link_directories(/usr/local/lib)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/lib")

set(${PROJECT_NAME}_LIB_INSTALL_PATH "${CMAKE_INSTALL_FULL_LIBDIR}/${PROJECT_FILE_NAME}/")
set(CMAKE_INSTALL_RPATH ${${PROJECT_NAME}_LIB_INSTALL_PATH})
set(${PROJECT_NAME}_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(${PROJECT_NAME}_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(${PROJECT_NAME}_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include PARENT_SCOPE)
set(${PROJECT_NAME}_TEST_DIR ${PROJECT_SOURCE_DIR}/test)

set(${PROJECT_NAME}_HEADERS
    ${${PROJECT_NAME}_INCLUDE_DIR}/spring.hpp
    ${${PROJECT_NAME}_INCLUDE_DIR}/spring_common.hpp
    ${${PROJECT_NAME}_SOURCE_DIR}/spring_lcl.hpp)

set(${PROJECT_NAME}_SOURCES
    ${${PROJECT_NAME}_SOURCE_DIR}/spring.cpp)

add_library(${PROJECT_FILE_NAME} SHARED
            ${${PROJECT_NAME}_HEADERS}
            ${${PROJECT_NAME}_SOURCES})
target_link_libraries(${PROJECT_FILE_NAME}
                      glog
                      ${LIBRT}
                      ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS ${PROJECT_FILE_NAME}
        DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
        COMPONENT executables)

if (${PROJECT_NAME}_ENABLE_TESTS)

    add_executable(${PROJECT_NAME}_test
                   ${${PROJECT_NAME}_TEST_DIR}/spring_test.cpp)
    target_link_libraries(${PROJECT_NAME}_test
                          ${PROJECT_FILE_NAME}
                          gtest_main
                          mpmc_ring
                          mplreg-client-lib
                          glog)
    add_test(NAME ${PROJECT_NAME}_spring_test
             COMMAND ${PROJECT_NAME}_test)

endif()

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Amin")

include(CPack)

