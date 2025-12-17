cmake_minimum_required(VERSION 3.22)
project(PROJECT_NAME)

set(CMAKE_CXX_STANDARD 20)

# Must set the path to the main.cpp, for example: scripts/main.cpp if it is inside a folder
add_executable(${PROJECT_NAME} scripts/main.cpp
        scripts/Types.h
        scripts/gameLogic/item.h
        scripts/gameLogic/item.cpp
        scripts/gameLogic/quest.h
        scripts/gameLogic/quest.cpp
        scripts/gameLogic/gamePerson.h
        scripts/player.h
        scripts/player.cpp
        scripts/game.h
        scripts/game.cpp
        scripts/controllers/resourceManager.h
        scripts/errors.h
        scripts/controllers/resourceManager.cpp
        scripts/controllers/visualizer.h
        scripts/controllers/visualizer.cpp
        scripts/controllers/inputManager.h
        scripts/controllers/inputManager.cpp
        scripts/inputController.h
        scripts/inputController.cpp
        scripts/utils.h
        scripts/uiManager.cpp
        scripts/uiManager.h
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


target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARIES} ${SDL2_MIXER_LIBRARIES} ${SDL2_TTF_LIBRARIES})