#ifndef PROJECT_NAME_SHOPPERSON_H
#define PROJECT_NAME_SHOPPERSON_H
#include "notGamePerson.h"

struct ShopDef;

// Продавец: разговор с ним открывает его заведение — магазин, гостиницу или церковь (shops.json)
class ShopPerson : public NotGamePerson
{
public:
    ShopPerson(std::string name, std::string sprite, Point cell, Direction facing, const ShopDef& shop);

    [[nodiscard]] const ShopDef& getShop() const noexcept { return shop; }
    void talk(Game& game) override;

private:
    const ShopDef& shop;                              // живёт в GameData всю игру
};


#endif //PROJECT_NAME_SHOPPERSON_H
