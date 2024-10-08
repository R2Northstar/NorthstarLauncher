# NorthstarLauncher
![Build Status](https://github.com/R2Northstar/NorthstarLauncher/actions/workflows/ci.yml/badge.svg)

Launcher used to modify Titanfall 2 to allow Northstar mods and custom content to be loaded.

## Build

Check [BUILD.md](BUILD.md) for instructions on how to compile, you can also download [binaries built by GitHub Actions](https://github.com/R2Northstar/NorthstarLauncher/actions).

## Format

For project coding standards check out [STANDARDS.md](STANDARDS.md).

This project uses [clang-format](https://clang.llvm.org/docs/ClangFormat.html), make sure you run `clang-format -i --style=file --exclude=primedev/include primedev/*.cpp primedev/*.h` when opening a Pull Request. Check the tool's website for instructions on how to integrate it with your IDE.
