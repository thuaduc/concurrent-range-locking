# Concurrent Range Locking

In this research’s scope, we propose a new concurrent range lock design that leverages a probabilistic concurrent skip list. It consists of two main functions:

1. **try_lock**: The **try_lock** function searches for the required range ([start, end]) in the skip list. If an overlapping range exists, indicating another thread is modifying that range, the requesting thread must wait and retry. If not, the range is added to the list, signaling that the range is reserved.
2. **release_lock**: The **release_lock** function releases the lock by finding the address range in the skip list and removing it accordingly.

Our range lock design also utilizes the per-node lock instead of an interval lock, thus addressing the bottleneck problem of the spinlock-based range lock and maintaining the lock’s high level of performance.

```
Concurrent Range Lock
Level 3: head ------------------------------------->[28,33]->[36,41]------------------->[56,61]->[70,75]---> tail
Level 2: head ---------->[08,13]------------------->[28,33]->[36,41]->[42,47]---------->[56,61]->[70,75]---> tail
Level 1: head ->[00,05]->[08,13]->[14,19]---------->[28,33]->[36,41]->[42,47]->[49,54]->[56,61]->[70,75]---> tail
Level 0: head ->[00,05]->[08,13]->[14,19]->[21,26]->[28,33]->[36,41]->[42,47]->[49,54]->[56,61]->[70,75]---> tail
```

## Author

The open-source implementation belongs to Thua-Duc Nguyen <thuaduc.nguyen@tum.de>, produced as part of his bachelor's thesis. The project was under the advice of Lam-Duy Nguyen lamduy.nguyen@tum.de and under the supervision of Prof. Dr. Viktor Leis <leis@in.tum.de>.

## Build

Prerequisites:

- [googletest](https://github.com/google/googletest)
- [gbenchmark](https://github.com/google/benchmark)

```sh
make benchmark
make test
make example
```

## How to use

(refer to [app/example.c](app/example.c))

- Initialization

```C++
//                  type   maxlevel
ConcurrentRangeLock<uint16_t, 4> rangeLock{};
```

- Operations

```C++
rangeLock.tryLock(1, 10)
rangeLock.releaseLock(1, 10);
```

- Makefile.override mac

```sh
GTEST_DIR = /usr/local/Cellar/googletest/1.14.0/include
GTEST_LIB = /usr/local/Cellar/googletest/1.14.0/lib
CXX = clang++ -std=c++20 -O3

CXXX = -fsanitize=leak -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer

CFLAGS = -Wall -pedantic -Wextra -c -O3 -g

BENCHMARK_DIR = /usr/local/Cellar/google-benchmark/1.8.5/include
BENCHMARK_LIB = /usr/local/Cellar/google-benchmark/1.8.5/lib

```

- Makefile.override linux

```sh

```
