#include "user.hpp"

// Parameterized constructor
User::User(int fd, std::string name) {
    this->id = fd;
    this->Name = name;
}

void User::setName(const std::string& name) {
    this->Name = name;
}

std::string User::getName() const {
    return this->Name;
}

void User::setId(int fd) {
    this->id = fd;
}

int User::getId() const {
    return this->id;
}