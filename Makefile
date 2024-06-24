-include Makefile.override

SRCDIR_0 = src/v0/
SRCDIR_1 = src/v1/

BINDIR_0 = bin/v0/
BINDIR_1 = bin/v1/

APPDIR = app/

TESTDIR_0 = test/v0/
TESTDIR_1 = test/v1/

LDFLAGS = -L$(GTEST_LIB) -lgtest -lgtest_main -pthread


OBJS_0 = $(addprefix $(BINDIR_0), range_lock.o node.o)
OBJS_1 = $(addprefix $(BINDIR_1), range_lock.o)
GTEST = $(addprefix -I, $(GTEST_DIR))

.PHONY: all clean benchmark test debug

all: unittest benchmark

benchmark: $(BINDIR_0).a $(BINDIR_1).a
	$(CXX) -o $@ $(APPDIR)benchmark.cpp $^

debug: $(BINDIR_0).a $(BINDIR_1).a
	$(CXX) -o $@ $(APPDIR)debug.cpp $^

test: $(BINDIR_0).a $(BINDIR_1).a
	$(CXX) $(GTEST) -o test_v0 $(TESTDIR_0)/unittest.cpp $^ $(LDFLAGS)
	$(CXX) $(GTEST) -o test_v1 $(TESTDIR_1)/unittest.cpp $^ $(LDFLAGS)
	./test_v0
	./test_v1

$(BINDIR_0).a: $(OBJS_0)
	ar rcs $@ $^

$(BINDIR_0)%.o: $(SRCDIR_0)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

$(BINDIR_1).a: $(OBJS_1)
	ar rcs $@ $^

$(BINDIR_1)%.o: $(SRCDIR_1)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BINDIR_0)* $(BINDIR_1)* benchmark test_v0 test_v1 unittest *.dSYM
