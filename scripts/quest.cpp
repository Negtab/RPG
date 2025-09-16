#include "quest.h"
#include <algorithm>

Quest::Quest(std::string name, uint16_t reward, bool isMoney) : name(std::move(name)), reward(reward), isMoney(isMoney) {}

void Quest::addAim(const std::string& aim)
{
    aims[aim] = false;
}

void Quest::completeAim(const std::string& aim)
{
    if (aims.count(aim))
        aims[aim] = true;
}

bool Quest::isFinished() const
{
    return std::all_of(aims.begin(), aims.end(), [](const auto& pair) {
        return pair.second;
    });
}

