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

coordinator_base::coordinator_base(std::ostream& stream) :
    _events(1024),  // Arbitrary initial capacity that can grow anyway
    _stream(stream),
    _done(false),
    _worker(&coordinator_base::work, this) {}

coordinator_base::~coordinator_base() {
  _done = true;
  _worker.join();
}

void coordinator_base::work() {
  // There is no blocking `pop` on a boost::lockfree::queue because it would defeat its very purpose.
  // But we only care about `push` being lock-free, so we emulate a blocking `pop` with
  // this "poll, sleep" loop.
  while (true) {
    _events.consume_all([this](const event* event) {
      _stream << event->process_id << ',' << event->thread_id << ',' << event->time;
      for (auto& attribute : event->attributes) {
        _stream << ',' << attribute;
      }
      _stream  << '\n';  // No std::endl: don't flush each line, improve performance
      delete event;
    });

    if (_done) {
      break;
    } else {
      // Avoid using 100% CPU
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

// `boost::lockfree::queue` cannot contain anything smart (see "Requirements" in
// https://www.boost.org/doc/libs/1_76_0/doc/html/boost/lockfree/queue.html), so we have to
// use plain pointers. The resources acquired by this `new` are released in `work`.
// The destructor ensures this worker thread finishes dequeuing all events.

void coordinator_base::add_event(const event& e) {
  _events.push(new event(e));
}

}  // namespace chrones

#endif  // NO_CHRONES
