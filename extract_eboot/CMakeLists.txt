cmake_minimum_required(VERSION 2.8)

project(extract_eboot)

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
  c
  z
  m
)