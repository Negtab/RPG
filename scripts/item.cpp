#include "item.h"

Item::Item(int id, int8_t cost, std::string name) : id(id), cost(cost), name(std::move(name)) {}

int Item::getId() const { return id; }
void Item::setId(int newId) { this->id = newId; }

int8_t Item::getCost() const { return cost; }
void Item::setCost(int8_t newCost) { this->cost = newCost; }

std::string Item::getName() const { return name; }
void Item::setName(std::string newName) { this->name = std::move(newName); }

int8_t Item::getBonus() const { return bonus; }
void Item::setBonus(int8_t newBonus) { this->bonus = newBonus; }

SpecializationNames Item::getSpecName() const { return spec; }
void Item::setSpecName(SpecializationNames newSpec) { this->spec = newSpec; }

bool Item::getIsTemporary() const { return isTemporary; }
void Item::setIsTemporary(bool IsNewTemporary) { this->isTemporary = IsNewTemporary; }

bool Item::getIsDamageBoost() const { return isDamageBoost; }
void Item::setIsDamageBoost() { this->isDamageBoost = true; }

