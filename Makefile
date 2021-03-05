################### # Top-level rules # ###################

# -- Inventory Section -- #
EVAL_FILES   = main chrone
EVAL_CXXFILES = $(EVAL_FILES:=.cpp)
EVAL_OBJFILES = $(EVAL_FILES:=.o)

TEST_FILES   = chrone-tests chrone
TEST_CXXFILES = $(TEST_FILES:=.cpp)
TEST_OBJFILES = $(TEST_FILES:=.o)

EVAL_PRODUCT = chrones-eval
TEST_PRODUCT = chrones-tests

BUILD_DIR = build/
DEBUG_DIR = debug/
RELEASE_DIR = release/

#DEPS = 
# -- Compiler Section -- #
CC = g++
DEBUG_CXXFLAGS = -O0 -g -std=c++17 -Wall -Wextra -pedantic
OPTIM_CXXFLAGS = -O3 -std=c++17


LDFLAGS = -lgtest_main -lgtest -pthread -lm 
CXXRUNFLAGS = $(OPTIM_CXXFLAGS) $(LDFLAGS)
CXXDEBUGFLAGS = $(DEBUG_CXXFLAGS) $(LDFLAGS)

all: dirs test eval run

dirs:
	@mkdir -p $(BUILD_DIR)$(DEBUG_DIR)
	@mkdir -p $(BUILD_DIR)$(RELEASE_DIR)

eval: $(EVAL_OBJFILES)	
	$(CC) -o $(BUILD_DIR)$(DEBUG_DIR)$(EVAL_PRODUCT) $^ $(CXXDEBUGFLAGS)

run: $(EVAL_OBJFILES)	
	$(CC) -o $(BUILD_DIR)$(RELEASE_DIR)$(EVAL_PRODUCT) $^ $(CXXRUNFLAGS)

test: $(BUILD_DIR)$(TEST_OBJFILES)
	$(CC) -o $(BUILD_DIR)$(DEBUG_DIR)$(TEST_PRODUCT) $^ $(CXXDEBUGFLAGS)

# -- Base rules ----------
$(BUILD_DIR)%.o : %.cpp dirs
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -rf $(BUILD_DIR)
