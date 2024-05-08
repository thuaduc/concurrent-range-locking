# Concurrent Skiplist

A probabilistic skiplist that allows concurrent access and modification. The algorithm was inspired by _The Art of Multiprocessor Programming - Herlihy, M., & Shavit, N. (2008)_.

```
Level 3: head --->01--------------------------------------------->09--->Null
Level 2: head --->01--->02--------------->05--->06--------------->09--->Null
Level 1: head --->01--->02--------------->05--->06--->07--->08--->09--->Null
Level 0: head --->01--->02--->03--->04--->05--->06--->07--->08--->09--->Null
```

## Author

Thua-Duc Nguyen <ducnguyen.work.de@gmail.com>

## Build

Prerequisites for test and benchmark: [googletest](https://github.com/google/googletest) and [gbenchmark](https://github.com/google/benchmark)

```sh
make
```

## How to use

(refer to [app/example.c](app/example.c))

- Initialization

```C++
//                  keytype  valuetype height
ConcurrentSkipList<uint16_t, uint16_t, 4> csl{};
```

- Operations

```C++
csl.insertElement(1, 12345);
csl.searchElement(1)
csl.deleteElement(1);
```
