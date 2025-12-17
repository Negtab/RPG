#include "item.h"

Item::Item(int id, int8_t cost, std::string name) : id(id), cost(cost), name(std::move(name)) {}

int Item::getId() const { return id; }
void Item::setId(int newId) { this->id = newId; }

int8_t Item::getCost() const { return cost; }
void Item::setCost(int8_t newCost) { this->cost = newCost; }

std::string Item::getName() const { return name; }
void Item::setName(std::string newName) { this->name = std::move(newName); }

int8_t Item::getPower() const { return power; }
void Item::setPower(int8_t newPower) { this->power = newPower; }

bool Item::isConsumable() const { return true; }
void Item::setConsumable(bool newConsumable) { this->consumable = newConsumable; }

void Item::setEffectType(ItemEffectType newEffectType) { this->effectType = newEffectType; }
void Item::setTargetType(TargetType newTargetType) { this->targetType = newTargetType; }

ItemEffectType Item::getEffectType() const { return this->effectType; }
TargetType Item::getTargetType() const { return this->targetType; }
