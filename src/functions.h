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

#include <iostream>

#include "Message.h"

enum server_action {
  list_files,
  get_file = WITH_PARAM_ACTION | 0,
  abortar = WITH_PARAM_ACTION | 1,
  pausar = WITH_PARAM_ACTION | 2,
  resume = WITH_PARAM_ACTION | 3
};

sockaddr_in make_ip_address(int port, const std::string& ip_address = std::string());

std::string extract_string(Message message);

bool starts_with(const std::array<char, 1024>& array, std::string prefix);

void EncodeAction(Message& buffer, server_action action, std::string param = "");

server_action DecodeAction(const Message& buffer, std::string* param = nullptr);

std::string GenerateUid();

void sigusr_1_handler(int signo, siginfo_t* info, void* context);

void set_signals();

#endif
