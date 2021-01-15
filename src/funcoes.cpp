#include "funcoes.hpp"
#include <map>
#include <set>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

using namespace std;

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
