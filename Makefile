all:
	g++ -Wall -Wextra -O3 -std=c++20 main.cpp tcp_server.cpp user.cpp -o server

clean:
	rm -f server