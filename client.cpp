#include "client.hpp"
#include <string_view>
#include "room.hpp"
#include "validator.hpp"
#include <memory>
#include <sys/socket.h>
#include "tcp_server.hpp"

bool handle_login(ClientContext* ctx, std::string_view name_view,int fd) {
    // We MUST copy here because nick_view points to a temporary buffer
    // that will be erased soon.
    std::string name = std::string(name_view); 

    // Validation (Length check, illegal characters, etc.)
    size_t actual_chars = ChatProtocol::Validator::utf8_length(name_view);

if (actual_chars < 3 || actual_chars > 16) {
    return true;
}
    ctx->user->setName(name);
    ctx->state = ClientState::MENU;
    return true;
}
void handle_list(Server *pServer,int fd){
   pServer->send_room_list(fd);
}

void handle_create(Server* pServer,int fd){
    Room room(fd);
    pServer->addToRoomList(room);
}


bool process_client(
    Server* pServer,
    ClientContext* ctx,
    int fd,
    std::string_view raw_msg
){
    size_t space_pos = raw_msg.find(' ');
    std::string_view cmd_view;
    std::string_view data_view;

    if (space_pos == std::string_view::npos) {
        cmd_view = raw_msg;
    } else {
        cmd_view = raw_msg.substr(0, space_pos);
        data_view = raw_msg.substr(space_pos + 1);
    }

    if (ctx->state == ClientState::SET_NAME) {
        if (cmd_view == "/msg") {
            // handle_login will copy data_view into a string if needed
            handle_login(ctx,data_view,fd); 
        } 
    }
    else if(ctx->state==ClientState::MENU) {
        
        if (cmd_view=="/list") {
            handle_list(pServer,fd);
        }else if (cmd_view=="/join") {
         //handle join
         //handle_join(pServer,ctx,data_view,fd);
        }
        else if (cmd_view == "/create") {
            handle_create(pServer,fd);
        }
    }
    else if (ctx->state==ClientState::CHATTING) {
        if (cmd_view=="/msg") {
            //handle chatting
            //handle_chat(ctx,data_view,fd);
        }
    }
}