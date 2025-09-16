#ifndef PROJECT_NAME_ITEM_H
#define PROJECT_NAME_ITEM_H

#include "types.h"

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

    [[nodiscard]] int8_t getBonus() const;
    void setBonus(int8_t newBonus);

    [[nodiscard]] SpecializationNames getSpecName() const;
    void setSpecName(SpecializationNames newSpec);

    [[nodiscard]] bool getIsTemporary() const;
    void setIsTemporary(bool IsNewTemporary);

    [[nodiscard]] bool getIsDamageBoost() const;
    void setIsDamageBoost();

private:
    int id{0};
    int8_t cost{0}, bonus{0};
    std::string name;
    bool isDamageBoost{true}, isTemporary{false};
    SpecializationNames spec{SpecializationNames::None};
};


#endif //PROJECT_NAME_ITEM_H