# C++26 (mmap + std::string_view)

An attempt at an "optimal" solution that avoids unncessary memory allocations by parsing a `mmap`-ed file using `string_view`'s and heterogeneous lookup on an `unordered_map`.

To run it:

```
$ make
$ ./1brc /path/to/measurements.txt
```
