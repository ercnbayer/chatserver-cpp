#include "tcp_server.hpp"
#include "user.hpp"
#include <cstddef>
#include <cstdio>
#include <memory>
#include <unistd.h>
#include <utility>
#include "validator.hpp"

Server::Server() {
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, & opt, sizeof(opt));

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  int iBind = bind(server_fd, (struct sockaddr * ) & address, sizeof(address));
  if (iBind == -1) {
    perror("[ERR] Server bind error\n");
  }
  listen(server_fd, 20);
  set_nonblocking(server_fd);

  epoll_fd = epoll_create1(0);
  ev.events = EPOLLIN;
  ev.data.fd = server_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, & ev);

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

void Server::send_msg(int fd,
  const std::string & msg) {
  send(fd, msg.c_str(), msg.length(), 0);
}

bool Server::room_exists(int owner_id) {
  std::map < int, ClientContext > ::iterator it;
  for (it = clients.begin(); it != clients.end(); ++it) {
    if (it -> second.room_owner_id == owner_id) return true;
  }
  return false;
}

std::string Server::get_room_list() {
  std::map < int, int > room_map; // RoomID -> Occupant Count
  std::map < int, ClientContext > ::iterator it;
  for (it = clients.begin(); it != clients.end(); ++it) {
    if (it -> second.room_owner_id != -1) {
      room_map[it -> second.room_owner_id]++;
    }
  }

  if (room_map.empty()) return "No active rooms.\n";

  std::string list = "Active Rooms (by Owner ID):\n";
  std::map < int, int > ::iterator rit;
  for (rit = room_map.begin(); rit != room_map.end(); ++rit) {
    list += "- Room ID: " + std::to_string(rit -> first) + " (" + std::to_string(rit -> second) + " users)\n";
  }
  return list;
}

void Server::broadcast_to_room(int sender_fd,
  const std::string & msg) {
  int target_room = clients[sender_fd].room_owner_id;
  std::map < int, ClientContext > ::iterator it;
  for (it = clients.begin(); it != clients.end(); ++it) {
    // Broadcast only to users in the same room and in CHATTING state
    if (it -> second.state == CHATTING && it -> second.room_owner_id == target_room && it -> first != sender_fd) {
      send(it -> first, msg.c_str(), msg.length(), 0);
    }
  }
}

void Server::handle_client(int fd) {
  char buffer[1024];
  auto it = clients.find(fd);
  ClientContext * ctx = nullptr;

  if (it != clients.end()) {
    ctx = & (it -> second);
  }
  if (!ctx) {
    printf("[ERR] Invalid Socket Id:%d\n", fd);
  }

  while (1) {
    int iRead = read(fd, buffer, 1023);

    if (iRead == 0) {
      printf("Client Has Left Name %s Id:%d\n", ctx -> user -> getName().c_str(), ctx -> user -> getId());
    }
    if (iRead < 0) {
      if (iRead == EWOULDBLOCK || iRead == EAGAIN) {
        return;
      }
    }
    ctx -> message.append(buffer, iRead);
    // SECURITY: Check if adding new data exceeds the 4KB buffer limit
    if (ctx -> message.size() + iRead > 4096) {
      perror("Read error, closing connection");
      clients.erase(it);
      close(fd);
      return;
    }

    // Use string_view to avoid copying data for each message
    std::string_view buffer_view(ctx -> message);
    size_t offset = 0;

    while (true) {
      size_t pos = buffer_view.find("\r\n", offset);
      if (pos == std::string_view::npos) break;

      // Create a view of the message WITHOUT copying it
      std::string_view raw_msg = buffer_view.substr(offset, pos - offset);

      // Process using view (Validator and process_client must accept string_view)
      ChatProtocol::ValidationError err = ChatProtocol::Validator::validate_raw(raw_msg);

      if (err == ChatProtocol::ValidationError::NONE) {
        //process_client(ctx, fd, raw_msg);
      } else {
        // Handle error...
      }

      offset = pos + 2; // Move offset forward
    }

    // Only erase once at the very end (Much cheaper!)
    if (offset > 0) {
      ctx -> message.erase(0, offset);
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
                send_msg(c_fd, "Please enter your name:\r\n ");

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