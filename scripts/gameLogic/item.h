#ifndef PROJECT_NAME_ITEM_H
#define PROJECT_NAME_ITEM_H

#include "action.h"
#include "../types.h"

enum class ItemEffectType
{
    Heal,
    Damage,
    Buff,
    Debuff,
    RestoreMana,
    None
};

class Item
{
public:
    Item() = default;
    Item(int id, int8_t cost, std::string name);
    ~Item() = default;

    [[nodiscard]] int getId() const;
    void setId(int id);

    [[nodiscard]] int8_t getCost() const;
    void setCost(int8_t newCost);

    [[nodiscard]] std::string getName() const;
    void setName(std::string newName);

    [[nodiscard]] int8_t getPower() const;
    void setPower(int8_t value);

    [[nodiscard]] bool isConsumable() const;
    void setConsumable(bool value);

    [[nodiscard]] ItemEffectType getEffectType() const;
    [[nodiscard]] TargetType getTargetType() const;

    void setEffectType(ItemEffectType type);
    void setTargetType(TargetType type);
private:
    int id{0};
    int8_t cost{0}, power{0};
    std::string name;

    bool consumable{true};
    ItemEffectType effectType{ItemEffectType::None};
    TargetType targetType{TargetType::Self};
};


#endif //PROJECT_NAME_ITEM_H