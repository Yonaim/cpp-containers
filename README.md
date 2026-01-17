# cpp_containers

A C++98 reimplementation of core components of the C++ Standard Template Library (STL),
built from scratch with a focus on observable behavior parity with `std`, correctness,
and allocator/iterator fundamentals.

This project reproduces the public interface and behavior of standard containers like
`vector`, `map`, and `stack`, along with essential STL utilities (type traits, iterators,
algorithms, function objects).

* Targets C++98 only (no post-C++98 STL additions).

## Why this project

The STL embodies a dense set of engineering trade-offs: generic programming, iterator
abstraction, allocator-aware memory management, exception safety, and complexity guarantees.
Rebuilding the core pieces in C++98 is a practical way to understand why the APIs look the
way they do—and what it takes to match their observable behavior.

## Highlights

* Behavior parity mindset: designed to be compared against `std` via an external test runner
* C++98-first implementation (no modern language features)
* Allocator-aware containers & utilities
  * `allocator` / `allocator_traits` (implemented to the extent needed in a C++98 setting)
  * construction/destruction helpers and uninitialized memory algorithms
* Iterator infrastructure
  * iterator categories, traits, reverse iterators, and algorithm interoperability
* Associative containers powered by a Red–Black Tree
  * reusable RB-tree core used to implement `map`

## Build / usage

This repository is header-based and organized under `include/`.
Use a C++98-compatible toolchain.

```bash
c++ -std=c++98 -Wall -Wextra -Werror -I./include your_file.cpp
```

## Testing

Behavior parity checks against the standard library are done via a companion repository:

[cpp-containers-tester](https://github.com/Yonaim/cpp-containers-tester)  
: interactive CLI runner that builds and compares `ft` vs `std`

## Implemented components

### Containers

* `ft::vector`
* `ft::vector<bool>` specialization (bit-packed storage + proxy reference)
* `ft::map` (RB-tree based)
* `ft::stack` (container adaptor)

### Utilities / Foundations

* `ft::pair`
* type traits & SFINAE helpers (`enable_if`, `is_integral`, etc.)
* comparison functors (`less`, `equal_to`, `select1st`, …)
* core algorithms (`copy`, `copy_backward`, `fill`, `equal`, `lexicographical_compare`, …)
* uninitialized memory algorithms (`uninitialized_copy`, `uninitialized_fill_n`, …)

## Project layout

> The directories below refer to `include/ft/*`.

| Directory     | Description                                                                                    |
| ------------- | ---------------------------------------------------------------------------------------------- |
| `algorithm`   | Core generic algorithms                                                                        |
| `functional`  | Function objects and comparison functors                                                       |
| `type_traits` | Compile-time type inspection utilities                                                         |
| `iterator`    | Iterator implementations and supporting traits                                                 |
| `tree`        | Red–Black Tree implementation used by associative containers                                   |
| `memory`      | Memory utilities (`allocator`, `allocator_traits`, `uninitialized_*`, guards, destroy helpers) |
| `utility`     | General-purpose utilities such as `pair`, `swap`, and exception helpers                        |
| `container`   | STL-style containers (`vector`, `map`, `stack`, `vector<bool>`)                                |

## Design notes

### `ft::vector<T>`

* Uses the classic three-pointer layout (`_M_start`, `_M_finish`, `_M_end_of_storage`) to maintain `size()`/`capacity()` invariants.
* Range overloads use iterator-category dispatch to avoid multi-pass assumptions on input iterators.
* Reallocation constructs into fresh storage via `uninitialized_*` algorithms; partial construction is rolled back for exception safety.
* Insert/erase shift elements with `copy` / `copy_backward`-style moves and explicit `destroy` on the erased tail.

### `ft::vector<bool>`

* Bit-packed storage: logical size is in bits, while allocation is managed in words (`_M_end_of_storage` points to the end word).
* `operator[]` returns a proxy (`bit_reference`) to emulate `bool&` semantics over a single bit.
* Iteration is performed via a `(word pointer + bit offset)` representation (`_M_start`, `_M_finish`).
* Word-level operations are used where possible (e.g., fill/flip), with masking for partial words at boundaries.

### `ft::map<Key, T>`

* Built on a reusable Red–Black Tree core with a header/sentinel node to simplify `begin()/end()` and empty-tree edge cases.
* Ordering is defined by `Compare` on keys (value stored as `pair<const Key, T>`).
* Insert preserves iterator validity (no invalidation on insert), matching `std::map` expectations.
* Lookup (`find/lower_bound/upper_bound/equal_range`) follows standard BST traversal over RB-tree invariants.

### `ft::stack<T, Container>`

* A thin adaptor over an underlying container; operations forward to `Container` (default-style stack behavior).
* Only stack semantics are exposed (no iteration); relational operators delegate to the underlying container comparisons.

### `allocator / common infrastructure`

* Allocator storage uses a small base layer with a compile-time stateless/stateful branch so empty allocators can benefit from EBO, while stateful allocators are stored normally without changing the container API.
* Allocation and object lifetime are kept separate (allocate → `uninitialized_*`/construct → destroy → deallocate) to match STL-style lifetimes.
* Iterator/category dispatch is used across range operations to keep behavior correct (and efficient) across iterator types.

## References

* [CppReference](https://en.cppreference.com/index.html) (API/behavior reference; some pages include post-C++98 additions)
* [GCC libstdc++](https://github.com/gcc-mirror/gcc/tree/releases/gcc-3.0/libstdc%2B%2B-v3/include) (historical reference for C++98-era implementation strategies)
