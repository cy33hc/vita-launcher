cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(yoyo_launcher)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "YOYO Games Launcher")
set(VITA_TITLEID  "SMLA00002")
set(VITA_VERSION  "01.02")
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

include_directories(
  ../src
)

add_executable(${PROJECT_NAME}
  src/main.cpp
  ../src/fs.cpp
)

target_link_libraries(${PROJECT_NAME}
  stdc++
  c
  z
  SceLibKernel_stub
  SceSysmodule_stub
  SceShellSvc_stub
  m
  SceAppMgr_stub
  SceAppUtil_stub
)

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  FILE yoyo_version.txt yoyo_version.txt
  FILE version.txt version.txt
  FILE loader.bin loader.bin
  FILE loader2.bin loader2.bin
)
