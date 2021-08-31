cmake_minimum_required(VERSION 2.8)

project(extract_eboot)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

include_directories(
  src
)

add_executable(${PROJECT_NAME}
  src/fs.cpp
  src/main.cpp
  src/eboot.cpp
)

target_link_libraries(${PROJECT_NAME}
  stdc++
  z
  m
)
