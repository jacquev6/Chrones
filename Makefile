# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

############################
# Default top-level target #
############################

.PHONY: default
default: lint test

#############
# Inventory #
#############

# Source files
c++_header_files := $(wildcard c++/*.hpp)
c++_source_files := $(wildcard c++/*.cpp)
c++_test_source_files := $(wildcard c++/*-tests.cpp)

# Intermediate files
object_files := $(patsubst %.cpp,build/%.o,$(c++_source_files))

# Sentinel files
cpplint_sentinel_files := $(patsubst %,build/%.cpplint.ok,$(c++_header_files) $(c++_source_files))
test_sentinel_files := $(patsubst %,build/%.tests.ok,$(c++_test_source_files))

.PHONY: debug-inventory
debug-inventory:
	@echo "c++_header_files:\n$(c++_header_files)\n"
	@echo "c++_source_files:\n$(c++_source_files)\n"
	@echo "c++_test_source_files:\n$(c++_test_source_files)\n"
	@echo "object_files:\n$(object_files)\n"
	@echo "cpplint_sentinel_files:\n$(cpplint_sentinel_files)\n"
	@echo "test_sentinel_files:\n$(test_sentinel_files)\n"

###############################
# Secondary top-level targets #
###############################

.PHONY: compile
compile: $(object_files)

#######################
# Manual dependencies #
#######################

# Not worth automating with `g++ -M`: too simple

build/c++/chrones-tests: build/c++/chrones-tests.o build/c++/chrones.o
build/c++/chrones-tests.o: c++/chrones.hpp
build/c++/chrones.o: c++/chrones.hpp

########
# Lint #
########

.PHONY: lint
lint: $(cpplint_sentinel_files)

build/%.cpplint.ok: %
	@echo "cpplint $<"
	@mkdir -p $(dir $@)
	@cpplint --root=cpp --linelength=120 --filter=-build/include_subdir $<
	@touch $@

#########
# Tests #
#########

.PHONY: test
test: $(test_sentinel_files)

build/%-tests.cpp.tests.ok: build/%-tests
	@echo "$<"
	@mkdir -p $(dir $@)
	@rm -f build/$*-tests.*.chrones.csv
	@cd build/c++ && ../../$<
	@./chrones-report.py summaries build/$*-tests.*.chrones.csv >build/$*-tests.chrones.summaries.json
	@touch $@

########
# Link #
########

# Of test executables

build/%-tests: build/%-tests.o
	@echo "g++     $< -o $@"
	@mkdir -p $(dir $@)
	@g++ -g -fopenmp $^ -lgtest_main -lgtest -lboost_thread -o $@

###############
# Compilation #
###############

build/%.o: %.cpp
	@echo "g++  -c $< -o $@"
	@mkdir -p $(dir $@)
	@g++ -std=gnu++11 -Wall -Wextra -Wpedantic -Werror -g -fopenmp -c $< -o $@
