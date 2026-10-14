#include <SDL.h>
#include <filesystem>
#include "utils.h"
#include "game.h"
#include "../gameLogic/gameData.h"

int main(int argc, char* args [])
{
    SDLContext ctx;
    if (!initSDL(ctx, 900, 600))
        return 1;

    int exitCode = 0;
    try
    {
        const auto dataDir = std::filesystem::current_path().remove_filename() / "resources" / "Data";
        GameData::instance().loadAll(dataDir);

        Game game(ctx.window, ctx.renderer);
        game.run();
    }
    catch (const std::exception& e)
    {
        SDL_Log("Fatal error: %s", e.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game error", e.what(), ctx.window);
        exitCode = 1;
    }

    cleanupSDL(ctx);
    return exitCode;
}