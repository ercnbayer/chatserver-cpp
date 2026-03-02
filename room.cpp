#include "room.hpp"
#include <string>

Room::Room(int fd){
   this->RoomID=fd;
   this->RoomOwnerId=fd;
}

std::string Room::GetRoomId(){
   std::string roomId=std::to_string(RoomID);
   return roomId;
}