#include "tcp_server.hpp"
#include "user.hpp"
#include <cstddef>
#include <cstdio>
#include <memory>
#include <utility>

Server::Server() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    int iBind=bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    if (iBind==-1) {
        perror("[ERR] Server bind error\n");
    }
    listen(server_fd, 20);
    set_nonblocking(server_fd);

    epoll_fd = epoll_create1(0);
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    std::cout << "[LOG] Server started on port " << PORT << std::endl;
}

Server::~Server() {
    close(server_fd);
    close(epoll_fd);
}

void Server::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::send_msg(int fd, const std::string& msg) {
    send(fd, msg.c_str(), msg.length(), 0);
}

bool Server::room_exists(int owner_id) {
    std::map<int, ClientContext>::iterator it;
    for (it = clients.begin(); it != clients.end(); ++it) {
        if (it->second.room_owner_id == owner_id) return true;
    }
    return false;
}

std::string Server::get_room_list() {
    std::map<int, int> room_map; // RoomID -> Occupant Count
    std::map<int, ClientContext>::iterator it;
    for (it = clients.begin(); it != clients.end(); ++it) {
        if (it->second.room_owner_id != -1) {
            room_map[it->second.room_owner_id]++;
        }
    }

    if (room_map.empty()) return "No active rooms.\n";
    
    std::string list = "Active Rooms (by Owner ID):\n";
    std::map<int, int>::iterator rit;
    for (rit = room_map.begin(); rit != room_map.end(); ++rit) {
        list += "- Room ID: " + std::to_string(rit->first) + " (" + std::to_string(rit->second) + " users)\n";
    }
    return list;
}

void Server::broadcast_to_room(int sender_fd, const std::string& msg) {
    int target_room = clients[sender_fd].room_owner_id;
    std::map<int, ClientContext>::iterator it;
    for (it = clients.begin(); it != clients.end(); ++it) {
        // Broadcast only to users in the same room and in CHATTING state
        if (it->second.state == CHATTING && it->second.room_owner_id == target_room && it->first != sender_fd) {
            send(it->first, msg.c_str(), msg.length(), 0);
        }
    }
}

void Server::handle_client(int fd) {
    char buffer[1024];
    ssize_t bytes = read(fd, buffer, sizeof(buffer));

    if (bytes <= 0) {
        std::cout << "[DISCONNECT] FD " << fd << " left." << std::endl;
        clients.erase(fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        return;
    }

    ClientContext& client = clients[fd];

    // STEP 1: SET NAME
    if (client.state == SET_NAME) {
        std::string name(buffer, bytes);
        name.erase(name.find_last_not_of(" \n\r\t") + 1);
        client.user=std::make_unique<User>(fd,name);
        client.state = MENU;
        send_msg(fd, "Welcome " + client.user->getName() + "! ID: " + std::to_string(fd) + "\n");
        send_msg(fd, "Commands: /list, /create, /join {id}\n");
        return;
    }

    // STEP 2: MENU - Using memcmp for command checking
    if (client.state == MENU) {
        // Check for "/list" (5 bytes)
        if (bytes >= 5 && std::memcmp(buffer, "/list", 5) == 0) {
            send_msg(fd, get_room_list());
        } 
        // Check for "/create" (7 bytes)
        else if (bytes >= 7 && std::memcmp(buffer, "/create", 7) == 0) {
            client.room_owner_id = fd;
            client.state = CHATTING;
            send_msg(fd, "Room " + std::to_string(fd) + " created.\n");
        } 
        // Check for "/join " (6 bytes)
        else if (bytes >= 6 && std::memcmp(buffer, "/join ", 6) == 0) {
            try {
                // Buffer + 6 points to the start of the ID string
                std::string id_str(buffer + 6, bytes - 6);
                int target_id = std::stoi(id_str);
                
                if (room_exists(target_id)) {
                    client.room_owner_id = target_id;
                    client.state = CHATTING;
                    send_msg(fd, "Entered Room " + std::to_string(target_id) + "\n");
                    broadcast_to_room(fd, client.user->getName() + " joined.\n");
                } else {
                    send_msg(fd, "Error: Room not found.\n");
                }
            } catch (...) {
                send_msg(fd, "Invalid ID format.\n");
            }
        }
        // Optional: Binary Join Protocol ('J' + 4 byte int)
        else if (bytes >= 5 && buffer[0] == 'J') {
            int target_id;
            std::memcpy(&target_id, &buffer[1], sizeof(int));
            if (room_exists(target_id)) {
                client.room_owner_id = target_id;
                client.state = CHATTING;
                send_msg(fd, "Joined via Binary Protocol: " + std::to_string(target_id) + "\n");
            }
        }
    } 
    // STEP 3: CHATTING
    else if (client.state == CHATTING) {
        // Check for "/leave"
        if (bytes >= 6 && std::memcmp(buffer, "/leave", 6) == 0) {
            broadcast_to_room(fd, client.user->getName() + " left the room.\n");
            client.room_owner_id = -1;
            client.state = MENU;
            send_msg(fd, "Back to menu.\n");
        } else {
            std::string msg(buffer, bytes);
            broadcast_to_room(fd, "[" + client.user->getName() + "]: " + msg);
        }
    }
}
void Server::run() {
    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                struct sockaddr_in addr;
                socklen_t len = sizeof(addr);
                int c_fd = accept(server_fd, (struct sockaddr*)&addr, &len);
                set_nonblocking(c_fd);

                // Initialize new user with their socket FD as ID
                ClientContext new_client;
                new_client.user = nullptr;
                new_client.room_owner_id = -1;
                new_client.state = SET_NAME;

                clients[c_fd] = std::move(new_client);
                send_msg(c_fd, "Please enter your name: ");

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = c_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, c_fd, &ev);
                std::cout << "[NEW] Connection on FD " << c_fd << std::endl;
            } else {
                handle_client(events[n].data.fd);
            }
        }
    }
}