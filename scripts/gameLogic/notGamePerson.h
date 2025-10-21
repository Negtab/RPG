//
// Created by User on 21.10.2025.
//

#ifndef PROJECT_NAME_NOTGAMEPERSON_H
#define PROJECT_NAME_NOTGAMEPERSON_H
#include <string>
#include <vector>

class NotGamePerson
{
public:
    explicit NotGamePerson(std::string name = "", std::vector<std::string> dialogs = {});
    ~NotGamePerson() = default;

private:
    std::string name;
    std::vector<std::string> dialogs;
};

#endif //PROJECT_NAME_NOTGAMEPERSON_H