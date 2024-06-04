SRCDIR_0 = src/v0/
SRCDIR_1 = src/v1/

BINDIR_0 = bin/v0/
BINDIR_1 = bin/v1/

APPDIR = app/

GTESTDIR = /usr/local/Cellar/googletest/1.14.0/include
GTEST_DIR = /usr/local/Cellar/googletest/1.14.0/
GTEST_LIB = $(GTEST_DIR)lib/

CXX = clang++ -std=c++20 -g
CFLAGS = -Wall -Wextra -c -O3 -g
LDFLAGS = -L$(GTEST_LIB) -lgtest -lgtest_main -pthread

OBJS_0 = $(addprefix $(BINDIR_0), range_lock.o node.o)
OBJS_1 = $(addprefix $(BINDIR_1), range_lock_v1.o)

GTEST = $(addprefix -I, $(GTESTDIR))

.PHONY: all clean benchmark unittest

all: unittest benchmark

benchmark: $(BINDIR_0)v0.a $(BINDIR_1)v1.a
	$(CXX) -o $@ $(APPDIR)benchmark.cpp $^

unittest: $(BINDIR_0)v0.a
	$(CXX) $(LDFLAGS) $(GTEST) -o $@ $(APPDIR)test.cpp $^

$(BINDIR_0)v0.a: $(OBJS_0)
	ar rcs $@ $^

$(BINDIR_0)%.o: $(SRCDIR_0)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

$(BINDIR_1)v1.a: $(OBJS_1)
	ar rcs $@ $^

$(BINDIR_1)%.o: $(SRCDIR_1)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BINDIR_0)* $(BINDIR_1)* benchmark unittest *.dSYM
