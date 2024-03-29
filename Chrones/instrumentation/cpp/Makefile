# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

# https://stackoverflow.com/a/52603343/905845
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables

# https://stackoverflow.com/a/589300/905845
SHELL := /bin/bash -o pipefail -o errexit

############################
# Default top-level target #
############################

.PHONY: default
default: lint test

#############
# Inventory #
#############

# Source files
c++_header_files := $(wildcard *.hpp)
c++_source_files := $(wildcard *.cpp)
c++_test_source_files := $(wildcard *-tests.cpp)

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

########
# Lint #
########

.PHONY: lint
lint: $(cpplint_sentinel_files)

build/%.cpplint.ok: %
	@echo "cpplint $<"
	@mkdir -p $(dir $@)
	@cpplint --root=. --linelength=120 --filter=-build/include_subdir $<
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
	@cd build && OMP_NUM_THREADS=4 ../$< 2>&1 | tee ../$@.log
	@touch $@

########
# Link #
########

# Of test executables

build/%-tests: build/%-tests.o
	@echo "g++     $< -o $@"
	@mkdir -p $(dir $@)
	@g++ -g -fopenmp $^ -lgtest_main -lgtest -o $@

###############
# Compilation #
###############

build/%.o: %.cpp chrones.hpp
	@echo "g++  -c $< -o $@"
	@mkdir -p $(dir $@)
	@g++ -std=gnu++11 -Wall -Wextra -Wpedantic -Werror -Wsuggest-override -Weffc++ -g -O3 -fopenmp -c $< -o $@
