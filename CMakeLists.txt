cmake_minimum_required(VERSION 2.8)

project(rpclib)

    set(CMAKE_BUILD_TYPE "Release")

set(CLI_SRC "src/client.cpp")
set(SRV_SRC "src/server.cpp")

include_directories("include")

find_package(uvw CONFIG REQUIRED)
find_package(msgpack CONFIG REQUIRED)

add_executable(client ${CLI_SRC})
add_executable(server ${SRV_SRC})

target_link_libraries(client PRIVATE uvw::uvw msgpackc-cxx)
target_compile_features(client PUBLIC cxx_std_17)

target_link_libraries(server PRIVATE uvw::uvw msgpackc-cxx)
target_compile_features(server PUBLIC cxx_std_17)