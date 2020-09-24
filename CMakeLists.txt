cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(launcher)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Launcher")
set(VITA_TITLEID  "SMLA00001")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(modules/kernel)
add_subdirectory(modules/user)
add_subdirectory(modules/patch)

include_directories(
  modules/kernel
  modules/user
  src
)

# Add any additional library paths here
# ${CMAKE_CURRENT_BINARY_DIR} lets you use any library currently being built
link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

# Builds
FUNCTION(ADD_RESOURCES out_var)
  SET(result)
  FOREACH(ref_f ${ARGN})
  if (IS_ABSOLUTE "${ref_f}")
    SET(out_f "${ref_f}.o")
    STRING(REPLACE "${CMAKE_CURRENT_BINARY_DIR}/" "" in_f "${ref_f}")
    SET(work_dir "${CMAKE_CURRENT_BINARY_DIR}")
  else()
    SET(out_f "${CMAKE_CURRENT_BINARY_DIR}/${ref_f}.o")
    SET(in_f "${ref_f}")
    SET(work_dir "${CMAKE_SOURCE_DIR}")
  endif()
  GET_FILENAME_COMPONENT(out_dir ${out_f} DIRECTORY)
  ADD_CUSTOM_COMMAND(OUTPUT ${out_f}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
    COMMAND ${CMAKE_LINKER} -r -b binary -o ${out_f} ${in_f}
    DEPENDS ${ref_f}
    WORKING_DIRECTORY ${work_dir}
    COMMENT "Building resource ${out_f}"
    VERBATIM
    )
  LIST(APPEND result ${out_f})
  ENDFOREACH()
  SET(${out_var} "${result}" PARENT_SCOPE)
ENDFUNCTION()

# ugly hack
add_resources(vitashell_res
  ${CMAKE_CURRENT_BINARY_DIR}/modules/kernel/kernel.skprx
  ${CMAKE_CURRENT_BINARY_DIR}/modules/user/user.suprx
  ${CMAKE_CURRENT_BINARY_DIR}/modules/patch/patch.skprx
)

add_executable(${PROJECT_NAME}
  ${vitashell_res}
  src/launcher.cpp
  src/gui.cpp
  src/textures.cpp
  src/fs.cpp
  src/sfo.cpp
  src/game.cpp
  src/main.cpp
  src/pfs.c
  src/init.c
)

add_dependencies(${PROJECT_NAME} vitashell_user_stubs)
add_dependencies(${PROJECT_NAME} kernel.skprx)
add_dependencies(${PROJECT_NAME} user.suprx)
add_dependencies(${PROJECT_NAME} patch.skprx)

target_link_libraries(${PROJECT_NAME}
  vita2d
  png
  imgui_vita2d
  c
  z
  VitaShellUser_stub_weak
  VitaShellKernel2_stub_weak
  taihen_stub
  taihenForKernel_stub
  taihenModuleUtils_stub
  SceAppMgr_stub
  SceAppUtil_stub
  SceLibKernel_stub
  SceNpDrm_stub
  SceSysmodule_stub
  ScePower_stub
  SceNet_stub
  SceNetCtl_stub
  SceVshBridge_stub
  SceCommonDialog_stub
  SceDisplay_stub
  SceGxm_stub
  SceCtrl_stub
  SceTouch_stub
  SceAudio_stub
  SceHid_stub
  SceShaccCg_stub
  m
)

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  FILE noicon.png noicon.png
  FILE Ubuntu-R.ttf Ubuntu-R.ttf
)
