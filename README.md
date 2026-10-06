# Madridista Analytics

![CI](https://github.com/eeeaazyyy/madridistaAnalytics/actions/workflows/ci.yml/badge.svg)

A desktop application with Real Madrid match statistics and predictions, built with Qt 6 and C++23.

## Requirements

- Qt 6.8+ (developed with 6.11)
- CMake 3.25+
- A C++23 compiler: GCC 13+, MinGW 13+

## Build

Paths to Qt and the compiler are set in `CMakeUserPresets.json`, which inherits the presets from `CMakePresets.json`.

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```
