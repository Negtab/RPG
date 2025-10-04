cmake_minimum_required(VERSION 3.22)
project(PROJECT_NAME)

set(CMAKE_CXX_STANDARD 20)

# Must set the path to the main.cpp, for example: scripts/main.cpp if it is inside a folder
add_executable(${PROJECT_NAME} scripts/main.cpp
        scripts/Types.h
        scripts/item.h
        scripts/item.cpp
        scripts/quest.h
        scripts/quest.cpp
        scripts/gamePerson.h
        scripts/gamePerson.cpp
        scripts/player.h
        scripts/player.cpp
        scripts/game.h
        scripts/game.cpp
        scripts/ResourceManager.h
        scripts/errors.h
        scripts/resourceManager.cpp
        scripts/visualizer.h
        scripts/visualizer.cpp
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