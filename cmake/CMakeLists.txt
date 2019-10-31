cmake_minimum_required(VERSION 3.5)
project(cg2game)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_GENERATOR_PLATFORM x64)
set(SFML_DIR "../deps/lib64/linux/lib/cmake/SFML")

find_package(SFML 2.5 COMPONENTS graphics REQUIRED)

file(GLOB cg2game_SRC
    "../source/*.h"
    "../source/*.cpp"
)

add_executable(cg2game ${cg2game_SRC})

target_compile_options(cg2game PRIVATE -Wnarrowing)
target_link_libraries(cg2game sfml-graphics sfml-system sfml-window -lX11)
target_include_directories(cg2game PRIVATE ../deps/include)
set_target_properties(cg2game
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "../build/linux"
)
