all: client relay

client: client.cpp
	g++ -o client client.cpp -lpthread
relay: relay.cpp xml_parser
	g++ -o relay relay.cpp -I/usr/include/libxml2 -lxml2 -lpthread
