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

INCS_0 = $(addprefix -I, $(SRCDIR_0))
INCS_1 = $(addprefix -I, $(SRCDIR_1))

GTEST = $(addprefix -I, $(GTESTDIR))

.PHONY: all clean example test example_1

all: test

example: $(BINDIR_0)v0.a
	$(CXX) -o $@ $(APPDIR)example.cpp $^

example_1: $(BINDIR_1)v1.a
	$(CXX) -o $@ $(APPDIR)example_v1.cpp $^

test: $(BINDIR_0)v0.a
	$(CXX) $(GTEST) -o $@ $(APPDIR)test.cpp $^ $(INCS_0) $(LDFLAGS)


$(BINDIR_0)v0.a: $(OBJS_0)
	ar rcs $@ $^

$(BINDIR_0)%.o: $(SRCDIR_0)%.cpp
	$(CXX) $(CFLAGS) $(SRCDIR_0) $< -o $@

$(BINDIR_1)v1.a: $(OBJS_1)
	ar rcs $@ $^

$(BINDIR_1)%.o: $(SRCDIR_1)%.cpp
	$(CXX) $(CFLAGS) $(SRCDIR_1) $< -o $@

clean:
	rm -rf $(BINDIR_0)* $(BINDIR_1)* example example_1 test *.dSYM

