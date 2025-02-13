# Manual Testing

Automate everything, but remember how it actually works.

## Quick Reference

### Benchmark
Exercised in `.github/workflows/test_bazel.yaml`.
```shell
bazel test --config=benchmark //test/benchmark/...
```

### Coverage
Exercised in `.github/workflows/test_bazel.yaml`.
```shell
bazel coverage --instrument_test_targets --instrumentation_filter="^//..." --combined_report=lcov //...
genhtml bazel-out/_coverage/_coverage_report.dat --output-directory coverage
chromium coverage/index.html
```

### Fuzz Test
Exercised in `.github/workflows/test_bazel.yaml` with guess limit.
```shell
bazel run --config=asan-libfuzzer //test/fuzz:grow_mpop_fuzz_test_full
# Full fuzz tests never terminate.
# Get better info if there's an error with: -c dbg
```

### Build System Parity
```shell
make
make test | grep 'Test *#' | wc -l
bazel test -- //... | grep '^//' | wc -l
# Bazel and CMake should run the same number of tests on Linux.
# If CMake is short by 5, try rerunning Bazel tests with `-//test/benchmark/...`
# to exclude benchmarks. CMake only runs those if Google Benchmark is installed.

# Run ./test/manual/parity.fildesh to find Bazel tests
# that don't have a CMake counterpart.
# It will list some extra tests due to an imperefct mapping,
# but the missing tests are almost certainly in the list as well!
```

### Valgrind Debugging
```shell
bazel test -c dbg --test_output=all --cache_test_results=no --run_under='valgrind --trace-children=yes' //test/src:kve_test
```

### Leak Check
```shell
bazel build //src/bin:fildesh
./bazel-bin/src/bin/fildesh test/manual/leak_check.fildesh
# Don't worry if the `xargs ... grep ...` command fails.
```

### Lint
Exercised in `.github/workflows/lint_manual.yaml`.
```shell
bazel build //src/bin:fildesh
./bazel-bin/src/bin/fildesh test/manual/lint.fildesh
```

### SIMD Instructions
```shell
objdump -M intel --disassemble=find_FildeshMascii bld/test/benchmark/strcspn_benchmark | cut -f3
```

### TinyCC Compilation
Exercised in `.github/workflows/lint_manual.yaml`.
```shell
cmake -B bld -D CMAKE_C_COMPILER=tcc
cmake --build bld
```
