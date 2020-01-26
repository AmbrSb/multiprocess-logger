cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(MPLogger VERSION 0.0.1 DESCRIPTION "Multiprocess Fast Logger")

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(InstallRequiredSystemLibraries)
include(GNUInstallDirs)

find_package(Git)
find_package(Threads)

find_program(MAKE_EXE NAMES make)
find_program(GIT_EXE NAMES git)


add_compile_options(
    # -Wall -Wpedantic -fexceptions
    "$<$<CONFIG:Debug>:-O0;-g3;-ggdb>"
    "$<$<CONFIG:Release>:-O2>"
)

if(GIT_FOUND AND NOT EXISTS "${CMAKE_SOURCE_DIR}/.git")
    message(WARNING "Configre Git!")
elseif(GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list --max-count=1 HEAD
            OUTPUT_VARIABLE GIT_REVISION
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            ERROR_QUIET
            )
    if(NOT ${GIT_REVISION} STREQUAL "")
            string(STRIP ${GIT_REVISION} GIT_REVISION)
    endif()
endif()

add_compile_definitions(
    FORTIFY_SOURCE=2
    "$<$<CONFIG:Debug>:MALLOC_CHECK_=3;_GLIBCXX_DEBUG>"
)


include_directories(/usr/local/include include)
link_directories(/usr/local/lib)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/deps")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/registry")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/spring")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/extractor")

enable_testing()

include(Utils)
include(Opts)
include(${PROJECT_NAME}Deps)

set(GSL_SOURCE_DIR "${CMAKE_BINARY_DIR}/gsl-src")
include_directories(${GSL_SOURCE_DIR}/include)

add_subdirectory(registry)
add_subdirectory(ring)
add_subdirectory(spring)
add_subdirectory(extractor)

include(CTest)
include(CPack)