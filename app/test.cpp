#include "bot.hpp"
#include "utils.hpp"

#include <iomanip>
#include <iostream>

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: ./test [board dimension] [in-a-row]\n";
    return 1;
  }

  std::cout << std::fixed << std::setprecision(2);

  unsigned dim = std::stoul(argv[1]);
  unsigned in_a_row = std::stoul(argv[2]);

  return 0;
}
