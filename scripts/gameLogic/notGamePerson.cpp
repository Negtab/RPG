#include "notGamePerson.h"
NotGamePerson::NotGamePerson(std::string name, std::vector<std::string> dialogs) : name(std::move(name)), dialogs(std::move(dialogs)) {}
