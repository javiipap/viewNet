#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <iostream>

sockaddr_in make_ip_address(int port,
                            const std::string& ip_address = std::string()) {
  in_addr address;
  if (ip_address.length())
    inet_aton(ip_address.c_str(), &address);
  else
    address.s_addr = htonl(INADDR_ANY);

  return {AF_INET, htons(port), address};
}

std::string extract_string(Message message) {
  std::string result;
  for (const auto ch : message.text) {
    result += ch;
    if (ch == '\0') break;
  }

  if (result[result.length() - 1] != '\0') {
    result += '\0';
  }

  return result;
}