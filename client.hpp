#pragma once
#include <iostream>
#include <memory>
#include "user.hpp"
// Protocol states
enum  ClientState { SET_NAME, MENU, CHATTING };

struct ClientContext {
    std::unique_ptr<User> user;
    int room_owner_id; // Room is identified by the owner's socket FD
    ClientState state;
    std::string message;
};
