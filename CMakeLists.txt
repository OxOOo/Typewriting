project(typewriting)
cmake_minimum_required(VERSION 3.5)

add_compile_options(--std=c++11)
add_compile_options(-O3)
add_compile_options(-Wall)
add_compile_options(-g)

add_executable(main src/main.cpp src/Typing.cpp)
target_link_libraries(main pthread)