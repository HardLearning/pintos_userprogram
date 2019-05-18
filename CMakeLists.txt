cmake_minimum_required(VERSION 3.8)
project(os)

set(CMAKE_CXX_STANDARD 11)
set(PROJECT_SOURCE_DIR "pintos/src")
include_directories("pintos/src" "pintos/src/userprog/" "pintos/src/lib"
        "pintos/src/lib/kernel" "pintos/src/lib/user" "pintos/src/threads"
        "pintos/src/utils" "pintos/src/filesys" "pintos/src/examples"
        "pintos/src/devices")

file(GLOB SOURCES "*/*.c" "*/*/*.c" "*/*.h" "*/*/*.h" main.cpp)
add_executable(os ${SOURCES})
