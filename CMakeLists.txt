cmake_minimum_required(VERSION 3.10)

project(cpp_rest_client VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(curl REQUIRED)

add_executable(cpp_rest_client main.cpp)

target_include_directories(cpp_rest_client PRIVATE ${CURL_INCLUDE_DIRS})
target_link_libraries(cpp_rest_client PRIVATE CURL::libcurl)