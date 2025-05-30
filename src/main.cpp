#include <iostream>

int main(int argc, char *argv[]) {
  if (argc == 0)
    std::cout << "X" << std::endl;
  else {
    std::cout << argv;
  }
  return 0;
}
