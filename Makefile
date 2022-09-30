all:clean server client

server:
	c++ server.cpp -l pthread -l pqxx -o server

client:
	c++ client.cpp -o client

clean:
	@rm server ||:
	@rm client ||:
tap_iface:
	sudo ip tuntap add dev tap0 mode tap
	sudo ip a add dev tap0 166.0.0.1/8