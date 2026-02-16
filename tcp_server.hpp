#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "user.hpp"

#define MAX_EVENTS 64
#define PORT 8080

// Protocol states
enum  ClientState { SET_NAME, MENU, CHATTING };

struct ClientContext {
     std::unique_ptr<User> user;
    int room_owner_id; // Room is identified by the owner's socket FD
    ClientState state;
};

class Server {
private:
    int server_fd;
    int epoll_fd;
    struct epoll_event ev, events[MAX_EVENTS];
    
    // Explicit map type without 'auto'
    std::map<int, ClientContext> clients;

    // Helper functions
    void send_msg(int fd, const std::string& msg);
    void broadcast_to_room(int sender_fd, const std::string& msg);
    bool room_exists(int owner_id);
    std::string get_room_list();
    void set_nonblocking(int fd);

public:
    Server();
    ~Server();
    void run();
    void handle_client(int fd);
};