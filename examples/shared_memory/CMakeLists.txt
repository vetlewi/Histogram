cmake_minimum_required(VERSION 3.14...3.22)
project(server_client VERSION 1.0 LANGUAGES C CXX)

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/CPM.cmake)

# ---- Dependencies ----
CPMAddPackage(NAME Histogram SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
find_package(ROOT COMPONENTS RIO Hist RHTTP)

add_executable(Server ${CMAKE_CURRENT_SOURCE_DIR}/server_sharedhistograms.cpp)
target_link_libraries(Server PRIVATE OCL::Histogram)
set_target_properties(Server PROPERTIES CXX_STANDARD 20)

if(ROOT_FOUND)
    add_executable(ClientHTTPServer ${CMAKE_CURRENT_SOURCE_DIR}/client_httpserver.cpp)
    target_link_libraries(ClientHTTPServer PRIVATE OCL::Histogram ROOT::Hist ROOT::RIO ROOT::RHTTP)
endif()
