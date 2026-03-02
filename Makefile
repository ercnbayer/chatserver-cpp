# Compiler and Flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g
TARGET = chat_server

# Object Files (The Pool)
# We MUST include room.o if you have room.cpp
OBJS = main.o tcp_server.o client.o  user.o room.o

# Linking Stage
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)
	@echo "[SUCCESS] Linking complete!"

# Compilation Stage

main.o: main.cpp tcp_server.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

tcp_server.o: tcp_server.cpp tcp_server.hpp client.hpp room.hpp user.hpp
	$(CXX) $(CXXFLAGS) -c tcp_server.cpp

client.o: client.cpp client.hpp tcp_server.hpp validator.hpp user.hpp room.hpp
	$(CXX) $(CXXFLAGS) -c client.cpp



user.o: user.cpp user.hpp
	$(CXX) $(CXXFLAGS) -c user.cpp

# This is where room.o is created. 
# Ensure room.cpp and room.hpp exist with these EXACT names.
room.o: room.cpp room.hpp
	$(CXX) $(CXXFLAGS) -c room.cpp

clean:
	rm -f *.o $(TARGET)

re: clean all

.PHONY: all clean re