cmake_minimum_required(VERSION 3.21)
project(Histogram VERSION 1.2.0 LANGUAGES C CXX)

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
include(cmake/CPM.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.8.0")

find_package(Threads REQUIRED)
find_package(ROOT COMPONENTS RIO Hist)

CPMAddPackage(
        NAME sanitizers-cmake
        GITHUB_REPOSITORY arsenm/sanitizers-cmake
        GIT_TAG 99e159ec9bc8dd362b08d18436bd40ff0648417b
        DOWNLOAD_ONLY YES
)

if ( sanitizers-cmake_ADDED )
    set(CMAKE_MODULE_PATH "${sanitizers-cmake_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
    find_package(Sanitizers)
endif()

# ---- Add source files ----
set(headers
    ${CMAKE_CURRENT_SOURCE_DIR}/include/histogram/Histogram1D.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/histogram/Histogram2D.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/histogram/Histogram3D.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/histogram/Histograms.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/histogram/MamaWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/histogram/ThreadSafeHistograms.h
)
set(sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/histogram/Histogram1D.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/histogram/Histogram2D.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/histogram/Histogram3D.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/histogram/Histograms.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/histogram/MamaWriter.cpp
)

if(ROOT_FOUND)
    list(APPEND headers ${CMAKE_CURRENT_SOURCE_DIR}/include/histogram/RootWriter.h)
    list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/src/histogram/RootWriter.cpp)
endif()

add_library(${PROJECT_NAME} ${headers} ${sources})

set_target_properties(Histogram PROPERTIES CXX_STANDARD 17)

target_include_directories(Histogram
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/histogram>
)

if(ROOT_FOUND)
    target_link_libraries(Histogram PRIVATE ROOT::RIO ROOT::Hist)
endif()


# ---- Create an installable target ----
# this allows users to install and find the library via `find_package()`.

# the location where the project's version header will be placed should match the project's regular
# header paths
string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)

packageProject(
        NAME ${PROJECT_NAME}
        VERSION ${PROJECT_VERSION}
        NAMESPACE OCL
        BINARY_DIR ${PROJECT_BINARY_DIR}
        INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
        INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
        VERSION_HEADER "${VERSION_HEADER_LOCATION}"
        COMPATIBILITY SameMajorVersion
)