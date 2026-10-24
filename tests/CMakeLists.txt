# Новый файл с тестами — просто добавь его в этот список
add_executable(rpg_tests
        gamePerson_test.cpp
        hero_test.cpp
)

target_link_libraries(rpg_tests PRIVATE rpg_core GTest::gtest_main)
# Тесты читают настоящие данные игры: heroes.json, items.json и т.д.
target_compile_definitions(rpg_tests PRIVATE RPG_DATA_DIR="${CMAKE_SOURCE_DIR}/resources/Data")

# SDL подключён через DLL: без них рядом rpg_tests.exe не запустится
file(GLOB SDL_DLLS ${CMAKE_SOURCE_DIR}/SDL2*/x86_64-w64-mingw32/bin/*.dll)
add_custom_command(TARGET rpg_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL_DLLS} $<TARGET_FILE_DIR:rpg_tests>)

include(GoogleTest)
gtest_discover_tests(rpg_tests DISCOVERY_TIMEOUT 30)
