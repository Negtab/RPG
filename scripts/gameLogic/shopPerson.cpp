#include "shopPerson.h"

#include "gameData.h"
#include "../main/game.h"

ShopPerson::ShopPerson(std::string name, std::string sprite, Point cell, Direction facing, const ShopDef& shop)
    : NotGamePerson(std::move(name), std::move(sprite), cell, facing), shop(shop) {}

void ShopPerson::talk(Game& game)
{
    game.openShop(shop);
}
