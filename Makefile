-include Makefile.override

SRCDIR = src/
INCDIR = inc/
BINDIR = bin/
APPDIR = app/

CXX = clang++ -std=c++20
CFLAGS = -Wall -pedantic -Wextra -c -O3
LDFLAGS = -L$(GTEST_LIB) -lgtest -lgtest_main -pthread

OBJS = $(addprefix $(BINDIR), range_lock.o node.o)
INCS = $(addprefix -I, $(INCDIR))
GTEST = $(addprefix -I, $(GTESTDIR))

.PHONY: all clean example test benchmark

all: test

benchmark: $(BINDIR)csl.a
	$(CXX) -o $@ $(APPDIR)benchmark.cpp $^

example: $(BINDIR)csl.a
	$(CXX) -o $@ $(APPDIR)example.cpp $^

test: $(BINDIR)csl.a
	$(CXX) $(GTEST) -o $@ $(APPDIR)test.cpp $^ $(INCS) $(LDFLAGS)

$(BINDIR)csl.a: $(OBJS)
	ar rcs $@ $^

$(BINDIR)%.o: $(SRCDIR)%.cpp
	$(CXX) $(CFLAGS) $(INCS) $< -o $@

clean:
	rm -rf $(BINDIR)* example test example.dSYM

