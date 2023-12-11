cmake_minimum_required(VERSION 3.5)

project(socketapp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23 -g -O0")


# Gather all source files in src/client and src/server
file(GLOB SERVER_SOURCES "src/server/*.*")
file(GLOB CLIENT_SOURCES "src/client/*.*")
file(GLOB COMMON_SOURCES "src/common/*.*")


add_executable(server ${SERVER_SOURCES} ${COMMON_SOURCES})
add_executable(client ${CLIENT_SOURCES} ${COMMON_SOURCES})

# Link the Winsock library
if(WIN32)
  target_link_libraries(server wsock32 ws2_32)
  target_link_libraries(client wsock32 ws2_32)
endif()

target_include_directories(server PUBLIC src/common)
target_include_directories(client PUBLIC src/common)