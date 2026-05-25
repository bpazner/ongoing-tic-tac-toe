#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>
#include <vector>

static constexpr int BACKLOG = 8;

// Relay all data from src to dst until the connection closes
static void relay(int src, int dst) {
  char buf[256];
  while (true) {
    ssize_t n = recv(src, buf, sizeof(buf), 0);
    if (n <= 0) {
      break;
    }
    ssize_t sent = 0;
    while (sent < n) {
      ssize_t s = send(dst, buf + sent, n - sent, 0);
      if (s <= 0) {
        return;
      }
      sent += s;
    }
  }
}

// Pair two clients and relay moves between them until one disconnects
static void handle_pair(int fd1, int fd2) {
  printf("Pairing fds %d and %d\n", fd1, fd2);

  // Randomly assign sides
  bool fd1_is_x = rand() % 2 == 0;
  send(fd1, fd1_is_x ? "X" : "O", 1, 0);
  send(fd2, fd1_is_x ? "O" : "X", 1, 0);

  std::thread t([fd1, fd2] {
    // Send from fd1 to fd2
    relay(fd1, fd2);
    shutdown(fd2, SHUT_RDWR);
  });

  // Send from fd2 to fd1
  relay(fd2, fd1);
  shutdown(fd1, SHUT_RDWR);

  // End session
  t.join();
  close(fd1);
  close(fd2);

  printf("Session ended for fds %d and %d\n", fd1, fd2);
}

int main(int argc, char** argv) {
  srand(time(nullptr));
  int port = 12345;
  if (argc >= 2) {
    port = std::stoi(argv[1]);
    if (argc >= 3) {
      fprintf(stderr, "Usage: %s [port]\n", argv[0]);
      return 1;
    }
  }

  // Create TCP socket
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket");
    return 1;
  }

  // Allow reuse of port after restart
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // Bind to all interfaces on port
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }
  if (listen(server_fd, BACKLOG) < 0) {
    perror("listen");
    return 1;
  }

  printf("Listening on port %d\n", port);

  // Accept clients in pairs
  while (true) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    int a = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (a < 0) {
      perror("accept");
      continue;
    }
    printf("Player 1 connected: %s\n", inet_ntoa(client_addr.sin_addr));

    int b = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (b < 0) {
      perror("accept");
      close(a);
      continue;
    }
    printf("Player 2 connected: %s\n", inet_ntoa(client_addr.sin_addr));

    std::thread([a, b] { handle_pair(a, b); }).detach();
  }

  close(server_fd);
  return 0;
}
