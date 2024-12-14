cmake_minimum_required( VERSION 3.25 )
project( TrySDL3 C )

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

add_executable(${PROJECT_NAME} trysdl3.c)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

