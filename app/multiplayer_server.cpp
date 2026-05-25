#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

static constexpr int BACKLOG = 8;

struct WaitingClient {
  int fd;
  uint8_t board_dimension;
  uint8_t in_a_row;
};

static std::vector<WaitingClient> waiting;
static std::mutex waiting_mutex;

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

// Read settings from client, then match with a waiting client or wait
static void handle_client(int fd) {
  // Read 2-byte settings, {board_dimension, in_a_row}
  uint8_t settings[2];
  if (recv(fd, settings, sizeof(settings), MSG_WAITALL) != sizeof(settings)) {
    fprintf(stderr, "Failed to read settings from fd %d\n", fd);
    close(fd);
    return;
  }
  uint8_t board_dimension = settings[0];
  uint8_t in_a_row = settings[1];
  printf("fd %d wants board=%d, in_a_row=%d\n", fd, board_dimension, in_a_row);

  // Check if a matching client is already waiting
  int match_fd = -1;
  {
    std::lock_guard<std::mutex> lock(waiting_mutex);
    for (auto it = waiting.begin(); it != waiting.end(); ++it) {
      if (it->board_dimension == board_dimension && it->in_a_row == in_a_row) {
        match_fd = it->fd;
        waiting.erase(it);
        break;
      }
    }
    if (match_fd < 0) {
      waiting.push_back({fd, board_dimension, in_a_row});
    }
  }

  if (match_fd >= 0) {
    handle_pair(match_fd, fd);
    return;
  }

  // No match yet, poll until matched or disconnected
  while (true) {
    // Check if fd was matched and removed from waiting list
    {
      std::lock_guard<std::mutex> lock(waiting_mutex);
      bool still_waiting = false;
      for (const auto& w : waiting) {
        if (w.fd == fd) {
          still_waiting = true;
          break;
        }
      }
      if (!still_waiting) {
        // Matched by another thread
        return;
      }
    }

    // Check if client disconnected
    char buf;
    ssize_t n = recv(fd, &buf, 1, MSG_DONTWAIT);
    if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
      // Remove from waiting list
      std::lock_guard<std::mutex> lock(waiting_mutex);
      waiting.erase(
          std::remove_if(waiting.begin(), waiting.end(),
                         [fd](const WaitingClient& w) { return w.fd == fd; }),
          waiting.end());
      close(fd);
      printf("Waiting client fd %d disconnected\n", fd);
      return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
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

  while (true) {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (fd < 0) {
      perror("accept");
      continue;
    }
    printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));
    std::thread([fd] { handle_client(fd); }).detach();
  }

  close(server_fd);
  return 0;
}
