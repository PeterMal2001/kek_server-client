all:clean server client

server:
	c++ server.cpp -Wall -l pthread -l pqxx -o server

client:
	c++ client.cpp -Wall -o client

clean:
	@rm server ||:
	@rm client ||:
	
tap_iface:
	sudo ip tuntap add dev tap0 mode tap
	sudo ip a add dev tap0 166.0.0.1/8

git_add:
	git add server.cpp frames.hpp crypto.hpp client.cpp ThreeFish.hpp aux.hpp consts.hpp Makefile
