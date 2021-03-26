################### # Top-level rules # ###################

# -- Inventory Section -- #
EVAL_FILES   = main chrone
EVAL_CXXFILES = $(EVAL_FILES:=.cpp)
EVAL_HXXFILES = chrone.hpp
EVAL_OBJFILES = $(EVAL_FILES:=.o)

TEST_FILES   = chrone-tests chrone
TEST_CXXFILES = $(TEST_FILES:=.cpp)
TEST_OBJFILES = $(TEST_FILES:=.o)

TIMING_FILES = chrone-timingtests chrone
TIMING_CXXFILES = $(TIMING_FILES:=.cpp)
TIMING_OBJFILES = $(TIMING_FILES:=.o)

EVAL_PRODUCT = chrones-eval
TEST_PRODUCT = chrones-tests
TIMINGTEST_PRODUCT = chrones-timingtests

BUILD_DIR = build/
DEBUG_DIR = debug/
RELEASE_DIR = release/

#DEPS = 
# -- Compiler Section -- #
CC = g++
DEBUG_CXXFLAGS = -O0 -g -std=c++17 -Wall -Wextra -pedantic
OPTIM_CXXFLAGS = -O3 -std=c++17


LDFLAGS_TESTS = -lgtest_main -lgtest -pthread -lm 
LDFLAGS = -lm 
CXXRUNFLAGS = $(OPTIM_CXXFLAGS) $(LDFLAGS)
CXXDEBUGFLAGS = $(DEBUG_CXXFLAGS) $(LDFLAGS)
CXXTESTSFLAGS = $(OPTIM_CXXFLAGS) $(LDFLAGS_TESTS)


all: test timing-test eval run

.PHONY: eval
eval: $(EVAL_OBJFILES)	
	@mkdir -p $(BUILD_DIR)$(DEBUG_DIR)
	$(CC) -o $(BUILD_DIR)$(DEBUG_DIR)$(EVAL_PRODUCT) $^ $(CXXDEBUGFLAGS)
	@echo "the executable is there" $(BUILD_DIR)$(DEBUG_DIR)$(EVAL_PRODUCT)

.PHONY: run
run: $(EVAL_OBJFILES)	
	@mkdir -p $(BUILD_DIR)$(RELEASE_DIR)
	$(CC) -o $(BUILD_DIR)$(RELEASE_DIR)$(EVAL_PRODUCT) $^ $(CXXRUNFLAGS)
	@echo "the executable is there" $(BUILD_DIR)$(RELEASE_DIR)$(EVAL_PRODUCT)

.PHONY: test
test: $(TEST_OBJFILES)
	@mkdir -p $(BUILD_DIR)$(DEBUG_DIR)
	$(CC) -o $(BUILD_DIR)$(DEBUG_DIR)$(TEST_PRODUCT) $^ $(CXXTESTSFLAGS)
	@echo "the test is there" $(BUILD_DIR)$(DEBUG_DIR)$(TEST_PRODUCT)

.PHONY: timing-test
timing-test: $(TIMING_CXXFILES)
	@mkdir -p $(BUILD_DIR)$(RELEASE_DIR)
	$(CC) -o $(BUILD_DIR)$(RELEASE_DIR)$(TIMINGTEST_PRODUCT) $^ $(CXXRUNFLAGS)
	@echo "the test is there" $(BUILD_DIR)$(RELEASE_DIR)$(TIMINGTEST_PRODUCT)


# -- Base rules ----------
$(BUILD_DIR)%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: lint
lintfiles= $(EVAL_CXXFILES) $(EVAL_HXXFILES)

lint: $(lintfiles)
	@echo cpplint $<
	@cpplint --linelength=120 --filter=-legal/copyright,-readability/fn_size $(lintfiles) # | (grep -v "^Done processing" || true) | tee build/$*.cpplint.log

$(lintfiles):
	@cpplint --linelength=120 --filter=-legal/copyright,-readability/fn_size $@ -i # | (grep -v "^Done processing" || true) | tee build/$*.cpplint.log

clean:
	rm -f *.o
	rm -f *.csv
	rm -rf $(BUILD_DIR)

