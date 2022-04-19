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
set(VITA_VERSION  "03.64")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

add_definitions(-DSQLITE_OS_OTHER=1 -DSQLITE_TEMP_STORE=3 -DSQLITE_THREADSAFE=0)

include_directories(
  src
  sqlite-3.6.23.1
)

add_executable(${PROJECT_NAME}
  src/launcher.cpp
  src/gui.cpp
  src/textures.cpp
  src/fs.cpp
  src/sfo.cpp
  src/game.cpp
  src/main.cpp
  src/inifile.c
  src/vita_sqlite.c
  src/db.cpp
  src/config.cpp
  src/eboot.cpp
  src/iso.cpp
  src/cso.cpp
  src/style.cpp
  src/ime_dialog.cpp
  src/net.cpp
  src/ftpclient.cpp
  src/audio.c
  src/ogg.c
  src/vitaaudiolib.c
  src/zip.c
  src/unzip.c
  src/ioapi.c
  src/sha1.c
  src/updater.cpp
  sqlite-3.6.23.1/sqlite3.c
)

target_link_libraries(${PROJECT_NAME}
  vorbisfile
  vorbis
  ogg
  vita2d
  png
  imgui_vita2d
  stdc++
  c
  z
  SceCommonDialog_stub
  SceLibKernel_stub
  SceDisplay_stub
  SceGxm_stub
  SceSysmodule_stub
  SceCtrl_stub
  SceTouch_stub
  SceAudio_stub
  SceHid_stub
  SceNet_stub
  SceNetCtl_stub
  SceHttp_stub
  SceSsl_stub
  ScePspnetAdhoc_stub
  ScePromoterUtil_stub
  SceShellSvc_stub
  m
  SceAppMgr_stub
  SceAppUtil_stub
  SceShaccCg_stub
)

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  FILE thumbnails_db/a2600.db thumbnails/a2600.db
  FILE thumbnails_db/a5200.db thumbnails/a5200.db
  FILE thumbnails_db/a7800.db thumbnails/a7800.db
  FILE thumbnails_db/aLynx.db thumbnails/aLynx.db
  FILE thumbnails_db/bandai.db thumbnails/bandai.db
  FILE thumbnails_db/c64.db thumbnails/c64.db
  FILE thumbnails_db/gaw.db thumbnails/gaw.db
  FILE thumbnails_db/gb.db thumbnails/gb.db
  FILE thumbnails_db/gba.db thumbnails/gba.db
  FILE thumbnails_db/gbc.db thumbnails/gbc.db
  FILE thumbnails_db/ggear.db thumbnails/ggear.db
  FILE thumbnails_db/mame2k.db thumbnails/mame2k.db
  FILE thumbnails_db/mame2k3.db thumbnails/mame2k3.db
  FILE thumbnails_db/mdrive.db thumbnails/mdrive.db
  FILE thumbnails_db/msx2.db thumbnails/msx2.db
  FILE thumbnails_db/msystem.db thumbnails/msystem.db
  FILE thumbnails_db/n64.db thumbnails/n64.db
  FILE thumbnails_db/nec.db thumbnails/nec.db
  FILE thumbnails_db/neogeo.db thumbnails/neogeo.db
  FILE thumbnails_db/neogeopc.db thumbnails/neogeopc.db
  FILE thumbnails_db/nes.db thumbnails/nes.db
  FILE thumbnails_db/ps1.db thumbnails/ps1.db
  FILE thumbnails_db/saturn.db thumbnails/saturn.db
  FILE thumbnails_db/snes.db thumbnails/snes.db
  FILE thumbnails_db/tgrafx.db thumbnails/tgrafx.db
  FILE thumbnails_db/vectrex.db thumbnails/vectrex.db
  FILE thumbnails_db/amiga.db thumbnails/amiga.db
  FILE thumbnails_db/sega32x.db thumbnails/sega32x.db
  FILE thumbnails_db/segacd.db thumbnails/segacd.db
  FILE thumbnails_db/msx1.db thumbnails/msx1.db
  FILE thumbnails_db/neogeocd.db thumbnails/neogeocd.db
  FILE thumbnails_db/scummvm.db thumbnails/scummvm.db
  FILE thumbnails_db/dreamcast.db thumbnails/dreamcast.db
  FILE thumbnails_db/mame_mapping.db thumbnails/mame_mapping.db
  FILE thumbnails_db/gms.db thumbnails/gms.db
  FILE icons/a2600.png icons/a2600.png
  FILE icons/a5200.png icons/a5200.png
  FILE icons/a7800.png icons/a7800.png
  FILE icons/aLynx.png icons/aLynx.png
  FILE icons/amiga.png icons/amiga.png
  FILE icons/bandai.png icons/bandai.png
  FILE icons/c64.png icons/c64.png
  FILE icons/dreamcast.png icons/dreamcast.png
  FILE icons/emulator.png icons/emulator.png
  FILE icons/favorites.png icons/favorites.png
  FILE icons/gaw.png icons/gaw.png
  FILE icons/gb.png icons/gb.png
  FILE icons/gba.png icons/gba.png
  FILE icons/gbc.png icons/gbc.png
  FILE icons/ggear.png icons/ggear.png
  FILE icons/homebrew.png icons/homebrew.png
  FILE icons/mame2k.png icons/mame2k.png
  FILE icons/mame2k3.png icons/mame2k3.png
  FILE icons/mdrive.png icons/mdrive.png
  FILE icons/msx1.png icons/msx1.png
  FILE icons/msx2.png icons/msx2.png
  FILE icons/msystem.png icons/msystem.png
  FILE icons/n64.png icons/n64.png
  FILE icons/nec.png icons/nec.png
  FILE icons/neogeo.png icons/neogeo.png
  FILE icons/neogeocd.png icons/neogeocd.png
  FILE icons/neogeopc.png icons/neogeopc.png
  FILE icons/nes.png icons/nes.png
  FILE icons/original.png icons/original.png
  FILE icons/ports.png icons/ports.png
  FILE icons/ps1.png icons/ps1.png
  FILE icons/psmini.png icons/psmini.png
  FILE icons/psmobile.png icons/psmobile.png
  FILE icons/psp.png icons/psp.png
  FILE icons/saturn.png icons/saturn.png
  FILE icons/scummvm.png icons/scummvm.png
  FILE icons/sega32x.png icons/sega32x.png
  FILE icons/segacd.png icons/segacd.png
  FILE icons/snes.png icons/snes.png
  FILE icons/tgrafx.png icons/tgrafx.png
  FILE icons/utilities.png icons/utilities.png
  FILE icons/vectrex.png icons/vectrex.png
  FILE icons/vita.png icons/vita.png
  FILE icons/system.png icons/system.png
  FILE icons/gms.png icons/gms.png
  FILE noicon.png noicon.png
  FILE favorite.png favorite.png
  FILE square.png square.png
  FILE triangle.png triangle.png
  FILE circle.png circle.png
  FILE cross.png cross.png
  FILE start.png start.png
  FILE folder.png folder.png
  FILE selected.png selected.png
  FILE default_style.ini default_style.ini
  FILE music.ogg music.ogg
  FILE redbar.png redbar.png
  FILE greenbar.png greenbar.png
  FILE version.txt version.txt
  FILE head.bin head.bin
)
