#ifndef PROJECT_NAME_QUEST_H
#define PROJECT_NAME_QUEST_H

#include "types.h"
#include <map>

class Quest
{
public:
    explicit Quest(std::string name, uint16_t reward = 0, bool isMoney = false);
    ~Quest() = default;

    void addAim(const std::string& aim);
    void completeAim(const std::string& aim);
    [[nodiscard]] bool isFinished() const;

private:
    bool finished{false};
    bool isMoney{false};
    std::string name;
    std::map<std::string, bool> aims;
    uint16_t reward{0};
};



#endif //PROJECT_NAME_QUEST_H