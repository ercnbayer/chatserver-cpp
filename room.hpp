#pragma once

#include "user.hpp"
#include <string>
#include <vector>
class Room{
  public:
    int RoomID;
    int RoomOwnerId;
    std::string name;
    std::string GetRoomId();
    std::vector<User> Users;
    Room(int fd);
    ~Room();
};