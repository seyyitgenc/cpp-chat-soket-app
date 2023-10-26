cmake_minimum_required(VERSION 3.5)

project(socketapp)

# write me cmake file that generates server and client executable
# server executable should be called server

add_executable(server src/server.c)
add_executable(client src/client.c)

target_link_libraries(server pthread wsock32 ws2_32)
target_link_libraries(client pthread wsock32 ws2_32)