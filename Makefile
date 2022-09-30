all:
	c++ server.cpp -l pthread -o server
	c++ client.cpp -o client