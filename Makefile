-include Makefile.override

SRCDIR_0 = src/v0/
SRCDIR_1 = src/v1/
SRCDIR_2 = src/v2/
SRCDIR_3 = src/v3/
SRCDIR_4 = src/v4/

BINDIR_0 = bin/v0/
BINDIR_1 = bin/v1/
BINDIR_2 = bin/v2/
BINDIR_3 = bin/v3/
BINDIR_4 = bin/v4/

APPDIR = app/

TESTDIR_0 = test/v0/
TESTDIR_1 = test/v1/
TESTDIR_2 = test/v2/
TESTDIR_3 = test/v3/
TESTDIR_4 = test/v4/

LDFLAGS = -L$(GTEST_LIB) -lgtest -lgtest_main -pthread
BMFLAGS = -L$(BENCHMARK_LIB) -lbenchmark -lpthread

OBJS_0 = $(addprefix $(BINDIR_0), range_lock.o node.o atomic_reference.o)
OBJS_1 = $(addprefix $(BINDIR_1), range_lock.o node.o)
OBJS_2 = $(addprefix $(BINDIR_2), range_lock.o)
OBJS_3 = $(addprefix $(BINDIR_3), range_lock.o)
OBJS_4 = $(addprefix $(BINDIR_4), concurrent_tree.o keyrange.o treenode.o )

GTEST = $(addprefix -I, $(GTEST_DIR))

.PHONY: all $(MAKECMDGOALS)

all: test

v0: $(BINDIR_4)v.a
	$(CXX) -o $@ $(APPDIR)v0.cpp $^ -v

benchmark: $(BINDIR_0)v.a $(BINDIR_1)v.a $(BINDIR_2)v.a
	$(CXX) -o $@ $(APPDIR)benchmark.cpp $^

scalability: $(BINDIR_0)v.a $(BINDIR_1)v.a $(BINDIR_2)v.a
	$(CXX) -o $@ $(APPDIR)scalability.cpp $^

debug: $(BINDIR_0)v.a $(BINDIR_1)v.a $(BINDIR_2)v.a
	$(CXX) -o $@ $(APPDIR)debug.cpp $^

database:  $(BINDIR_0)v.a $(BINDIR_1)v.a $(BINDIR_2)v.a
	$(CXX) -o $@ $(APPDIR)database.cpp $^

test_main: $(BINDIR_2)v.a
	$(CXX) $(GTEST) -o test_v0 $(TESTDIR_0)unittest.cpp $^ $(LDFLAGS)
	./test_v0

test: $(BINDIR_3)v.a
#	$(CXX) $(GTEST) -o test_v0 $(TESTDIR_0)unittest.cpp $^ $(LDFLAGS)
#	$(CXX) $(GTEST) -o test_v1 $(TESTDIR_1)unittest.cpp $^ $(LDFLAGS)
#	$(CXX) $(GTEST) -o test_v2 $(TESTDIR_2)unittest.cpp $^ $(LDFLAGS)
	$(CXX) $(GTEST) -o test_v3 $(TESTDIR_3)unittest.cpp $^ $(LDFLAGS)
#	./test_v0
#	./test_v1
#	./test_v2
#	./test_v3

gtest: $(BINDIR_0)v.a $(BINDIR_1)v.a $(BINDIR_2)v.a $(BINDIR_3)v.a
	$(CXX) $(GTEST) -o gtest $(APPDIR)gtest.cpp $^ $(BMFLAGS)

# V0
$(BINDIR_0)v.a: $(OBJS_0)
	ar rcs $@ $^

$(BINDIR_0)%.o: $(SRCDIR_0)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

# V1
$(BINDIR_1)v.a: $(OBJS_1)
	ar rcs $@ $^

$(BINDIR_1)%.o: $(SRCDIR_1)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

# V2
$(BINDIR_2)v.a: $(OBJS_2)
	ar rcs $@ $^

$(BINDIR_2)%.o: $(SRCDIR_2)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

# V3
$(BINDIR_3)v.a: $(OBJS_3)
	ar rcs $@ $^

$(BINDIR_3)%.o: $(SRCDIR_3)%.cpp
	$(CXX) $(CFLAGS) $< -o $@

# V4
$(BINDIR_4)v.a: $(OBJS_4)
	ar rcs $@ $^

$(BINDIR_4)%.o: $(SRCDIR_4)%.cc
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BINDIR_0)* $(BINDIR_1)* $(BINDIR_2)* $(BINDIR_3)* $(BINDIR_4)* *.dSYM
	rm -rf v0 test_v0 test_v1 test_v2 test_v3
	rm -rf benchmark debug database scalability gtest