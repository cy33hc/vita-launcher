cmake_minimum_required(VERSION 2.8)

project(extract_eboot_iso)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fpermissive")

include_directories(
  src
)

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/fs.cpp
  src/eboot.cpp
)

target_link_libraries(${PROJECT_NAME}
  stdc++
)
