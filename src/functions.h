#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <iostream>

#include "Message.h"

sockaddr_in make_ip_address(int port,
                            const std::string& ip_address = std::string());

std::string extract_string(Message message);

bool starts_with(const std::array<char, 1024>& array, std::string prefix);