//
// Created by User on 21.10.2025.
//

#ifndef PROJECT_NAME_SHOPPERSON_H
#define PROJECT_NAME_SHOPPERSON_H
#include "gamePerson.h"

class ShopPerson : public GamePerson
{
public:
    ShopPerson() = default;
    ~ShopPerson() override = default;

    uint32_t getAttackPower(const Skill*) const override { return 0; }
    [[nodiscard]] uint32_t getDefense() const override { return 0; }
    [[nodiscard]] uint32_t getSpeed() const override { return 0; }
    [[nodiscard]] std::vector<int> getAvailableSkills() const override { return {}; }

private:
    std::map<int, int> itemKeys;
};


#endif //PROJECT_NAME_SHOPPERSON_H