#include <SDL.h>
#include "utils.h"

#include "game.h"

int main(int argc, char* args [])
{
    SDLContext ctx;
    if (!initSDL(ctx))
        return 1;

    Game game(ctx.renderer);
    game.run();

    cleanupSDL(ctx);
    return 0;
}
