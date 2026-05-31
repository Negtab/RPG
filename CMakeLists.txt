cmake_minimum_required(VERSION 3.22)
project(PROJECT_NAME)

set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")

set(CMAKE_CXX_STANDARD 20)

# Must set the path to the main.cpp, for example: scripts/main.cpp if it is inside a folder
add_executable(${PROJECT_NAME} scripts/main/main.cpp
        scripts/main/types.h
        scripts/gameLogic/quest.h
        scripts/gameLogic/quest.cpp
        scripts/gameLogic/gamePerson.h
        scripts/main/player.h
        scripts/main/player.cpp
        scripts/main/game.h
        scripts/main/game.cpp
        scripts/controllers/resourceManager.h
        scripts/main/errors.h
        scripts/controllers/resourceManager.cpp
        scripts/controllers/visualizer.h
        scripts/controllers/visualizer.cpp
        scripts/controllers/inputManager.h
        scripts/controllers/inputManager.cpp
        scripts/main/inputController.h
        scripts/main/inputController.cpp
        scripts/main/utils.h
        scripts/main/uiManager.cpp
        scripts/main/uiManager.h
        scripts/gameLogic/battle.cpp
        scripts/gameLogic/battle.h
        scripts/gameLogic/hero.h
        scripts/gameLogic/enemy.h
        scripts/gameLogic/hero.cpp
        scripts/gameLogic/enemy.cpp
        scripts/gameLogic/notGamePerson.cpp
        scripts/gameLogic/notGamePerson.h
        scripts/gameLogic/shopPerson.h
        scripts/gameLogic/shopPerson.cpp
        scripts/gameLogic/action.h
        scripts/online/Server.h
        scripts/online/Client.h
        scripts/online/server.cpp
        scripts/online/client.cpp
        scripts/online/NetworkRole.h
        scripts/online/networkPackets.h
        scripts/uiControlls/animPlayer.h
)

# --- SDL2 SETUP ---
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

set(SDL2_PATH "SDL2/x86_64-w64-mingw32")
set(SDL2_IMAGE_PATH "SDL2_image/x86_64-w64-mingw32")
set(SDL2_MIXER_PATH "SDL2_mixer/x86_64-w64-mingw32")
set(SDL2_TTF_PATH "SDL2_ttf/x86_64-w64-mingw32")

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_ttf REQUIRED)

include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2_IMAGE_INCLUDE_DIR})
include_directories(${SDL2_MIXER_INCLUDE_DIR})
include_directories(${SDL2_TTF_INCLUDE_DIR})


target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARIES} ${SDL2_MIXER_LIBRARIES} ${SDL2_TTF_LIBRARIES} ws2_32)