#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#define WITH_PARAM_ACTION 0x80
#define UUID_LENGTH 0x3c

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include "Message.h"

enum server_action {
  unknown = 0,
  list_files,
  get_file = WITH_PARAM_ACTION | 0,
  abortar = WITH_PARAM_ACTION | 1,
  pausar = WITH_PARAM_ACTION | 2,
  resumir = WITH_PARAM_ACTION | 3,
  retrieve_uuid = WITH_PARAM_ACTION | 4,
  close_connection = WITH_PARAM_ACTION | 5
};

sockaddr_in make_ip_address(int port, const std::string& ip_address = std::string());

std::string extract_string(const Message message);

void* cli(void* args);

bool starts_with(const std::string haystack, const std::string needle);

void EncodeAction(Message& buffer, server_action action, const std::string param = "");

server_action DecodeAction(const Message& buffer, std::string* param = nullptr);

std::string generate_uuid();

std::vector<std::string> split(const std::string& s);

void sigusr_1_handler(int signo, siginfo_t* info, void* context);

void sigusr_handler(int signo, siginfo_t* info, void* context);

void set_signals();

#endif
