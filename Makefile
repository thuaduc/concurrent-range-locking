SRCDIR = src/
INCDIR = inc/ 
GTESTDIR = /usr/local/Cellar/googletest/1.14.0/include
BINDIR = bin/
APPDIR = app/
GTEST_DIR = /usr/local/Cellar/googletest/1.14.0/
GTEST_LIB = $(GTEST_DIR)lib/

CXX = clang++ -std=c++20
CFLAGS = -Wall -Wextra -c -O3
LDFLAGS = -L$(GTEST_LIB) -lgtest -lgtest_main -pthread

OBJS = $(addprefix $(BINDIR), concurrentSkipList.o node.o)
INCS = $(addprefix -I, $(INCDIR))
GTEST = $(addprefix -I, $(GTESTDIR))

.PHONY: all clean example test

all: test

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

