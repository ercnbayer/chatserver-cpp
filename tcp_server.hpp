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
#include <vector>
#include "room.hpp"
#include "client.hpp"

#define MAX_EVENTS 64
#define PORT 8080


class Server {
private:
    int server_fd;
    int epoll_fd;
    struct epoll_event ev, events[MAX_EVENTS];
    

    std::map<int, ClientContext> clients;
    std::vector<Room> roomList;

    // Helper functions
    void send_msg(int fd, const std::string& msg);
    void broadcast_to_room(int sender_fd, const std::string& msg);
    bool room_exists(int owner_id);

    void set_nonblocking(int fd);
    bool handle_login(ClientContext* ctx, std::string_view name_view,int fd);
    void send_response(int fd,std::string_view message);


public:
    Server();
    ~Server();
    void run();
    void handle_client(int fd);
    void send_room_list(int fd);
    void addToRoomList(Room room);
};