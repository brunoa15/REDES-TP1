all:
	g++ -Wall -c common.cpp
	g++ -Wall -c funcoes.cpp
	g++ -Wall client.cpp common.o -lpthread -o cliente
	g++ -Wall server-mt.cpp common.o funcoes.o -lpthread -o servidor

clean:
	rm common.o client server server-mt
