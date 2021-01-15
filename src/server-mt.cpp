#include "common.h"
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <iostream>
#include <map>
#include <set>

#define BUFSZ 500

using namespace std;

void usage(int argc, char **argv) {
  printf("usage: %s <server port>\n", argv[0]);
  printf("example: %s 51511\n", argv[0]);
  exit(EXIT_FAILURE);
}

struct client_data {
  int csock;
  struct sockaddr_storage storage;
};

map<int, set<string> > tags_clientes;

bool checar_map_tags(int csock, string tag, int acao) {
  if (acao == 0) {
    if (tags_clientes[csock].count(tag)) {
      return false;
    }
    tags_clientes[csock].insert(tag);
    return true;
  }
  if (tags_clientes[csock].count(tag)) {
    tags_clientes[csock].erase(tag);
    return true;
  }
  return false;
}

bool checar_ascii(char c) {
  if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
    return true;
  }
  return false;
}

void enviar_mensagens(set<string> hashtags, string mensagem, int csock_autor) {
  map<int, set<string> >::iterator i;
  set<string>::iterator j;
  for (i = tags_clientes.begin(); i != tags_clientes.end(); i++) {
    for (j = hashtags.begin(); j != hashtags.end(); j++) {
      if (i->second.count(*j) && i->first != csock_autor) {
        send(i->first, mensagem.c_str(), mensagem.size(), 0);
        break;
      }
    }
  }
}

void buscar_hashtags(char *buf, int csock) {
  string strbuf(buf);
  string bufaux;
  set<string> hashtags;
  for (int i=0; i<strbuf.size(); i++) {
    if (strbuf[i] == '#') {
      i++;
      while (checar_ascii(strbuf[i])) {
        bufaux += strbuf[i];
        i++;
      }
      if (strbuf[i] == ' ' || strbuf[i] == '\n') {
        hashtags.insert(bufaux);
      }
      bufaux.clear();
    }
  }
  if (!hashtags.empty()) {
    enviar_mensagens(hashtags, strbuf, csock);
  }
}

string tratar_tag(char *buf, char sinal) {
  string strbuf(buf);
  string bufaux;

  if (strbuf[0] != sinal || !checar_ascii(strbuf[1])) {
    bufaux.clear();
    return bufaux;
  }

  bufaux += strbuf[1];
  for (int i = 2; i<strbuf.size(); i++) {
    if (strbuf[i] == '\n') {
      // caso de sucesso
      return bufaux;
    }
    
    if (!checar_ascii(strbuf[i])) {
      bufaux.clear();
      return bufaux;
    }

    bufaux += strbuf[i];
  }

  bufaux.clear();
  return bufaux;
}

string tratar_mensagem(char *buf, int csock) {
  if (strcmp(buf, "##kill\n") == 0) {
    exit(EXIT_SUCCESS);
  }

  string subscribe = tratar_tag(buf, '+');
  string unsubscribe = tratar_tag(buf, '-');
  buscar_hashtags(buf, csock);

  if (!subscribe.empty() && unsubscribe.empty()) {
    string buf_envio;
    if (checar_map_tags(csock, subscribe, 0)) {
      buf_envio = "subscribed +";
      buf_envio += subscribe;
      buf_envio += "\n";
    } else {
      buf_envio = "already subscribed +";
      buf_envio += subscribe;
      buf_envio += "\n";
    }
    return buf_envio;
  }

  if (subscribe.empty() && !unsubscribe.empty()) {
    string buf_envio;
    if(checar_map_tags(csock, unsubscribe, 1)) {
      buf_envio = "unsubscribed -";
      buf_envio += unsubscribe;
      buf_envio += "\n";
    } else {
      buf_envio = "not subscribed -";
      buf_envio += unsubscribe;
      buf_envio += "\n";
    }
    return buf_envio;
  }

  return "";
}

void * client_thread(void *data) {
  struct client_data *cdata = (struct client_data *)data;
  struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

  char caddrstr[BUFSZ];
  addrtostr(caddr, caddrstr, BUFSZ);
  printf("[log] connection from %s\n", caddrstr);

  set<string> tags;
  tags_clientes.insert(make_pair(cdata->csock, tags));

  char buf_recebido[BUFSZ];
  string buf_envio;
  size_t count;

  while(1) {
    memset(buf_recebido, 0, BUFSZ);
    count = recv(cdata->csock, buf_recebido, BUFSZ - 1, 0);
    buf_envio = tratar_mensagem(buf_recebido, cdata->csock);
    
    count = send(cdata->csock, buf_envio.c_str(), buf_envio.size() + 1, 0);
    if (count != buf_envio.size() + 1) {
      logexit("send");
    }
  }
  
  close(cdata->csock);
  pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(argc, argv);
  }

  struct sockaddr_storage storage;
  if (0 != server_sockaddr_init(argv[1], &storage)) {
    usage(argc, argv);
  }

  int s = socket(storage.ss_family, SOCK_STREAM, 0);
  if (s == -1) {
    logexit("socket");
  }

  int enable = 1;
  if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
    logexit("setsockopt");
  }

  struct sockaddr *addr = (struct sockaddr *)(&storage);
  if (0 != bind(s, addr, sizeof(struct sockaddr))) {
    logexit("bind");
  }

  if (0 != listen(s, 10)) {
    logexit("listen");
  }

  char addrstr[BUFSZ];
  addrtostr(addr, addrstr, BUFSZ);
  printf("bound to %s, waiting connections\n", addrstr);

  while (1) {
    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);

    int csock = accept(s, caddr, &caddrlen);
    if (csock == -1) {
      logexit("accept");
    }

    struct client_data *cdata = (struct client_data *)malloc(sizeof(*cdata));
    if (!cdata) {
      logexit("malloc");
    }
    cdata->csock = csock;
    memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

    pthread_t tid;
    pthread_create(&tid, NULL, client_thread, cdata);
  }

  exit(EXIT_SUCCESS);
}
