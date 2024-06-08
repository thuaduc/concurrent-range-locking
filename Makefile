-include Makefile.override

SRCDIR_0 = src/v0/
SRCDIR_1 = src/v1/

BINDIR_0 = bin/v0/
BINDIR_1 = bin/v1/

APPDIR = app/

CFLAGS = -Wall -pedantic -Wextra -c -O3 -g $(addprefix -I, $(GTESTDIR))
LDFLAGS = -L$(GTEST_LIB) -lgtest_main -lgtest -pthread

OBJS_0 = $(addprefix $(BINDIR_0), range_lock.o node.o)
OBJS_1 = $(addprefix $(BINDIR_1), range_lock.o)

.PHONY: all clean benchmark unittest

all: unittest benchmark

benchmark: $(BINDIR_0).a $(BINDIR_1).a
	$(CXX) -o $@ $(APPDIR)benchmark.cpp $^ $(LDFLAGS)

unittest: $(BINDIR_0).a
	$(CXX) -o $@ $(APPDIR)unittest.cpp $^ $(LDFLAGS)

$(BINDIR_0).a: $(OBJS_0)
	ar rcs $@ $^

$(BINDIR_0)%.o: $(SRCDIR_0)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

$(BINDIR_1).a: $(OBJS_1)
	ar rcs $@ $^

$(BINDIR_1)%.o: $(SRCDIR_1)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BINDIR_0)* $(BINDIR_1)* benchmark unittest *.dSYM
