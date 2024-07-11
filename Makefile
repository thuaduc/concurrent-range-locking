-include Makefile.override

SRCDIR_0 = src/v0/
SRCDIR_1 = src/v1/
SRCDIR_2 = src/v2/

BINDIR_0 = bin/v0/
BINDIR_1 = bin/v1/
BINDIR_2 = bin/v2/

APPDIR = app/

TESTDIR_0 = test/v0/
TESTDIR_1 = test/v1/
TESTDIR_2 = test/v2/


LDFLAGS = -L$(GTEST_LIB) -lgtest -lgtest_main -pthread

OBJS_0 = $(addprefix $(BINDIR_0), range_lock.o node.o)
OBJS_1 = $(addprefix $(BINDIR_1), range_lock.o)
OBJS_2 = $(addprefix $(BINDIR_2), range_lock.o node.o atomic_reference.o)

GTEST = $(addprefix -I, $(GTEST_DIR))

.PHONY: all clean benchmark test debug v2 database

all: unittest benchmark

v2:  $(BINDIR_2).a
	$(CXX) -o $@ $(APPDIR)v2.cpp $^

benchmark: $(BINDIR_0).a $(BINDIR_1).a $(BINDIR_2).a
	$(CXX) -o $@ $(APPDIR)benchmark.cpp $^

debug: $(BINDIR_0).a $(BINDIR_1).a $(BINDIR_2).a
	$(CXX) -o $@ $(APPDIR)debug.cpp $^

database:  $(BINDIR_0).a $(BINDIR_1).a
	$(CXX) -o $@ $(APPDIR)database.cpp $^

test2: $(BINDIR_2).a
	$(CXX) $(GTEST) -o test_v2 $(TESTDIR_2)/unittest.cpp $^ $(LDFLAGS)
	./test_v2

test: $(BINDIR_0).a $(BINDIR_1).a
	$(CXX) $(GTEST) -o test_v0 $(TESTDIR_0)/unittest.cpp $^ $(LDFLAGS)
	$(CXX) $(GTEST) -o test_v1 $(TESTDIR_1)/unittest.cpp $^ $(LDFLAGS)
	$(CXX) $(GTEST) -o test_v2 $(TESTDIR_2)/unittest.cpp $^ $(LDFLAGS)
	./test_v0
	./test_v1
	./test_v2

$(BINDIR_0).a: $(OBJS_0)
	ar rcs $@ $^

$(BINDIR_0)%.o: $(SRCDIR_0)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

$(BINDIR_1).a: $(OBJS_1)
	ar rcs $@ $^

$(BINDIR_1)%.o: $(SRCDIR_1)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

$(BINDIR_2).a: $(OBJS_2)
	ar rcs $@ $^

$(BINDIR_2)%.o: $(SRCDIR_2)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BINDIR_0)* $(BINDIR_1)* $(BINDIR_2)* benchmark v2 debug test_v0 test_v1 test_v2 v2 debug *.dSYM database
