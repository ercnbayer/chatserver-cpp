#pragma once
#include <string>

class User {
private:
    std::string Name;
    int id; // This represents the socket file descriptor (FD)

public:
    // Constructors
    User(int fd, std::string name);
    
    // Destructor
    ~User() = default;

    // Getter and Setter methods for Name
    void setName(const std::string& name);
    std::string getName() const;

    // Getter and Setter methods for ID (Socket Descriptor)
    void setId(int fd);
    int getId() const;
};