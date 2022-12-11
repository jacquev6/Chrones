#include <unistd.h>

#include <chrones.hpp>


CHRONABLE("program-1")

int main() {
  CHRONE();

  chdir("tutu");
}
