// Copyright 2020-2022 Laurent Cabaret
// Copyright 2020-2022 Vincent Jacques

#include <unistd.h>

#include <chrones.hpp>


CHRONABLE("program-1")

int main() {
  CHRONE();

  chdir("tutu");
}
