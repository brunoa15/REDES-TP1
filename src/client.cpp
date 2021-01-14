#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

using namespace std;
#define BUFSZ 1024

struct client_data {
  int csock;
  struct sockaddr_storage storage;
};

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

void * send_thread(void *data) {
  struct client_data *cdata = (struct client_data *)data;
	char buf[BUFSZ];
	size_t count;

	while(1) {
		memset(buf, 0, BUFSZ);
		printf("> ");
		fgets(buf, BUFSZ-1, stdin);
		count = send(cdata->csock, buf, strlen(buf)+1, 0);
		if (count != strlen(buf)+1) {
			logexit("send");
		}
	}

  pthread_exit(EXIT_SUCCESS); 
}

void * recv_thread(void *data) {
  struct client_data *cdata = (struct client_data *)data;
  char buf[BUFSZ];
	unsigned total;
	size_t count;

  while(1) {
		memset(buf, 0, BUFSZ);
		total = 0;
		count = 0;
		while(1) {
			count = recv(cdata->csock, buf + total, BUFSZ - total, 0);
			total += count;
		}
  }

  pthread_exit(EXIT_SUCCESS); 
}

void a(int s) {
	struct client_data *cdata = (struct client_data *)malloc(sizeof(*cdata));
  if (!cdata) {
    logexit("malloc");
  }
  cdata->csock = s;
  struct sockaddr_storage cstorage;
  memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));
	
	// Leitura do teclado
  pthread_t tid1;
  pthread_create(&tid1, NULL, send_thread, cdata);
}

void b(int s) {
	struct client_data *cdata = (struct client_data *)malloc(sizeof(*cdata));
  if (!cdata) {
    logexit("malloc");
  }
  cdata->csock = s;
  struct sockaddr_storage cstorage;
  memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

	// Leitura do server
  pthread_t tid2;
  pthread_create(&tid2, NULL, recv_thread, cdata);
}

int main(int argc, char **argv) {
	if (argc < 3) {
		usage(argc, argv);
	}

	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		usage(argc, argv);
	}

	int s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(s, addr, sizeof(struct sockaddr))) {
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	printf("connected to %s\n", addrstr);

	a(s);
	b(s);

	while(1) {}

	exit(EXIT_SUCCESS);
}