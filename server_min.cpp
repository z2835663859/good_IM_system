#include "im_protocol.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

int main() {
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) { perror("socket"); return 1; }

  int yes = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(9000);

  if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("bind"); return 1;
  }
  if (listen(listen_fd, 16) < 0) {
    perror("listen"); return 1;
  }

  printf("[server] listen on 9000...\n");
  int client_fd = accept(listen_fd, nullptr, nullptr);
  if (client_fd < 0) { perror("accept"); return 1; }

  printf("[server] client connected\n");

  im::ByteBuffer buf;
  uint8_t tmp[4096];

  while (true) {
    ssize_t n = recv(client_fd, tmp, sizeof(tmp), 0);
    if (n > 0) {
      buf.append(tmp, (size_t)n);

      im::Packet pkt;
      while (im::Codec::try_decode(buf, pkt)) {
        printf("[server] decoded type=%u body=%s\n", pkt.type, pkt.body.c_str());
      }
    } else if (n == 0) {
      printf("[server] client closed\n");
      break;
    } else {
      perror("recv");
      break;
    }
  }

  close(client_fd);
  close(listen_fd);
  return 0;
}