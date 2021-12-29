#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>

#include <iostream>

#include "Message.h"

struct sockaddr_in make_ip_address(int port, const std::string& ip_address = std::string());

std::string extract_string(Message message);

bool starts_with(const std::array<char, 1024>& array, std::string prefix);

std::string DecodeAction();

std::string GenerateUid();

void sigusr_1_handler(int signo, siginfo_t* info, void* context);

void set_signals();

#endif
