# libft0cc

[![Travis](https://img.shields.io/travis/HertzDevil/0CC-FamiTracker/master.svg?style=flat-square&logo=travis)][travis-ci]
[![AppVeyor](https://img.shields.io/appveyor/ci/HertzDevil/0CC-FamiTracker/master.svg?style=flat-square&logo=appveyor)][appveyor]
[![Coveralls](https://img.shields.io/coveralls/github/HertzDevil/0CC-FamiTracker/master.svg?style=flat-square)][coveralls]

**libft0cc** comprises the core components of 0CC-FamiTracker. These components
are either exclusive to 0CC-FamiTracker, or rewritten based on the original
FamiTracker code, with the main objective that they work on all popular C++
compilers (GCC, Clang, Visual C++). Eventually all components that do not depend
on Win32 functionality shall be moved into libft0cc.

libft0cc is dual-licensed under MPL 2.0 and GPLv2.

## Building

libft0cc uses CMake to build the libraries:

```sh
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

libft0cc requires a fairly recent C++17 compiler, however [the targeted
compilers already support almost all of the C++17 features][cpp-support].

The 0CC-FamiTracker project does not use libft0cc's `CMakeLists.txt` file
itself; this will change in the future. The acutal Visual Studio project used by
0CC-FamiTracker is located at the `vc` directory.

## Testing

Building and running the unit tests requires [Google Test][gtest]. The CMake
build file automatically fetches the repository. Tests can be run with:

```sh
$ GTEST_COLOR=1 ctest -V
```

There is a long way to go before integration tests can be written for libft0cc.
However, as proofs of concept, many 0CC-FamiTracker source files can already be
compiled directly on GCC 7. The `../fuzz` directory demonstrates fuzz testing
with [American Fuzzy Lop][afl-fuzz], whereas the `../0ccft-ext` directory
compiles the Kraid's Hideout (NES) NSF on a *nix machine using 0CC-FamiTracker's
NSF exporter classes.

[cpp-support]: http://en.cppreference.com/w/cpp/compiler_support
[gtest]: https://github.com/google/googletest
[afl-fuzz]: http://lcamtuf.coredump.cx/afl/
[travis-ci]: https://travis-ci.org/HertzDevil/0CC-FamiTracker
[appveyor]: https://ci.appveyor.com/project/HertzDevil/0cc-famitracker
[coveralls]: https://coveralls.io/github/HertzDevil/0CC-FamiTracker?branch=master
