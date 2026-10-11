# Learning B+ Trees

I couldn't find an implementation that explained B+ tree internals clearly enough for me to learn from, so I decided to implement one myself. I wanted to understand how internal nodes guide a search, how leaves store entries, and how splitting nodes keeps the tree balanced as it grows. Building these pieces helped me understand why databases use B+ trees for indexes.

## Current scope

This is an in-memory C++17 B+ tree for integer keys and integer values. It supports:

- Inserting a new key-value pair.
- Looking up a key with `get()`, which returns `std::optional<int>`.
- Ignoring an insertion when the key already exists, preserving its original value.
- Splitting leaves and internal nodes, including creating a new root.
- Maintaining parent pointers and links between leaves.
- Validating the tree's structure with `validate(std::ostream&)`.

Internal nodes store separator keys and children. Leaves store the actual key-value entries in sorted order. All leaves are at the same depth. The deliberately small capacity—three children per internal node and three entries per leaf—makes splits easy to observe.

Updating existing values and deleting keys are outside the current scope. `del()` is declared but has no implementation. There is also no public range-scan API, disk storage, buffer pool, concurrency control, or recovery mechanism.

## What I learned about database indexes

In a database implementation, B+ tree nodes can be stored as fixed-size disk pages. Internal pages contain separator keys and child page identifiers. Comparing a search key with those separators tells the tree which child page to request next.

A **page** is a unit of stored data; a **frame** is a memory slot in the buffer pool that holds a page. The buffer pool manager finds a requested page in memory or loads it from disk into a frame. The tree chooses the next page by its identifier; the buffer pool manages where that page resides in memory. A cached page does not need another disk read. See [CMU's buffer pool material](https://15445.courses.cs.cmu.edu/fall2026/project1/).

The benefit is that each internal page can direct the search toward many children. This high fan-out keeps the tree shallow and reduces the number of pages needed for a lookup.

Leaf links also make range scans efficient. To scan `[start, end]`, find the leaf for `start`, locate the first entry at least `start`, and scan forward through the leaf links until a key exceeds `end`. There is no need to search from the root for every result. For `n` entries and `k` results, the usual in-memory cost is **O(log n + k)**. With disk storage, the relevant cost is the search-path pages and leaf pages visited, plus any table pages needed to fetch records.

Leaves contain the index entries; depending on the database and index type, their values may be records or references to records stored elsewhere. [CMU's B+ tree material](https://15445.courses.cs.cmu.edu/spring2026/project2/) illustrates child page identifiers, record identifiers, and linked-leaf iteration.

This project models the tree structure using C++ objects and pointers. The database storage discussion explains the motivation rather than a feature implemented here.

## Build and run

Requires CMake 3.16 or newer and a compiler supporting C++17.

From the project root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/btree_demo
```

After changing the code, from inside `build`:

```bash
cmake --build .
./btree_demo
```

For VS Code debugging, open the project root, install Microsoft's C/C++ extension, set a breakpoint, and press F5. The included `.vscode/launch.json` uses GDB and launches `build/btree_demo` with pretty-printing enabled.

## Tests

`main.cpp` checks the original 49-key insertion sequence and 1,000 keys inserted in ascending, descending, and shuffled order. The shuffle uses seed `42` so failures are reproducible.

After every insertion, the tests verify all previously inserted values and validate:

- Strict ordering of leaf keys and internal separators.
- Leaf keys staying within their separator ranges.
- Separators matching the minimum key of their right subtree.
- Node capacity, minimum occupancy, and child counts.
- Correct parent pointers, including a root with no parent.
- Equal depth for all leaves.
- Leaf links matching the tree's leaf order and ending at `nullptr`.

Additional lookup tests check missing keys in empty and populated trees, and distinguish missing values from valid values of `0` and `-1`.

These tests passed with structural validation enabled. Duplicate insertion behavior is implemented but does not yet have a dedicated test. The tests provide coverage for the current scope rather than a proof of correctness for every possible input.
