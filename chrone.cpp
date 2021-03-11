#include "chrone.hpp"


timer::timer(std::string label, chrone *handle, int64_t nb_of_iterations) {
    _label = label;
    _handle = handle;
    _nb_of_iterations = nb_of_iterations;
    _start_time = clk::now();
}


timer::~timer()  {
    std::chrono::time_point<clk> stop_time = clk::now();
    int64_t elapsed_time = ((stop_time - _start_time).count())/_nb_of_iterations;
    _handle->appendTimer(_label, elapsed_time);
}

chrone::chrone(std::string filename) {
    _filename = filename;
}

chrone::~chrone() {
    std::fstream file;
    file.open(_filename, std::ios::out);

    for (auto& sample : _rack) {
        file << sample.first << ";" << sample.second << "ns\n";
    }
}

void chrone::appendTimer(std::string label, int64_t elapsed_time) {
    _rack.push_back(std::pair<std::string, int64_t>(label, elapsed_time));
}

int64_t chrone::getSize() {
    return _rack.size();
}
