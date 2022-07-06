// Copyright 2020-2022 Laurent Cabaret
// Copyright 2020-2022 Vincent Jacques

#ifndef NO_CHRONES

#include "chrones.hpp"

#include <iostream>


namespace chrones {

std::chrono::steady_clock::time_point RealInfo::startup_time = std::chrono::steady_clock::now();

// The default CSV dialect used by Python's `csv` module interprets two double-quote characters
// as a single one inside a double-quoted string. This function replaces each double-quote
// character by two double-quote characters, and puts the result inside double-quotes.
std::string quote_for_csv(std::string s) {
  size_t start_pos = 0;
  while ((start_pos = s.find('"', start_pos)) != std::string::npos) {
    s.replace(start_pos, 1, "\"\"");
    start_pos += 2;
  }

  return "\"" + s + "\"";
}

}  // namespace chrones

#endif  // NO_CHRONES
