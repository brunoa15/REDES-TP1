#pragma once

#include <map>
#include <set>
#include <string>

using namespace std;

extern map<int, set<string> > tags_clientes;

bool checar_map_tags(int csock, string tag, int acao);

bool checar_ascii(char c);

void enviar_mensagens(set<string> hashtags, string mensagem, int csock_autor);

void buscar_hashtags(char *buf, int csock);

string tratar_tag(char *buf, char sinal);

string tratar_mensagem(char *buf, int csock);
