# CMake Style Guide

## Spacing
- 2 spaces per indent.
- 1 space between a conditional statement and the open parenthesis that follows it.
  - `if`, `elseif`
  - `foreach`, `while`
- 0 space after any other statements, function calls, etc.
  - `function`, `return`, `endfunction`
  - `else`, `endif`, `endforeach`, `endwhile`
  - `set`, `unset`
  - `string`, `list`
  - `add_executable`, `add_library`

## Indentation
- 1 indent for a continued argument list.
- 0 indent for a closing parenthesis on its own lines (Bazel style).

## CMake Invocation
Build and test the project from it's toplevel directory by running:
```shell
# Generate buildsystem in `bld/` directory.
cmake -B bld -D CMAKE_BUILD_TYPE=RelOnHost
# Compile.
cmake --build bld --config RelOnHost
# Test.
ctest --test-dir bld -C RelOnHost
```

If you're not in the project's toplevel directory, just specify it with an `-S` option to the first command.

The second and third commands only need the configuration specified as `RelOnHost` if a multi-config buildsystem generator was used (like Visual Studio on Windows).
