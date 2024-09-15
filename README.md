[![Bazel](https://github.com/fildesh/fildesh/actions/workflows/test_bazel.yaml/badge.svg)](https://github.com/fildesh/fildesh/actions/workflows/test_bazel.yaml)
[![CMake](https://github.com/fildesh/fildesh/actions/workflows/test_cmake.yaml/badge.svg)](https://github.com/fildesh/fildesh/actions/workflows/test_cmake.yaml)
\
[![Coverage Status](https://coveralls.io/repos/github/fildesh/fildesh/badge.svg?branch=trunk)](https://coveralls.io/github/fildesh/fildesh?branch=trunk)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/6377/badge)](https://www.bestpractices.dev/projects/6377)


# Fildesh: File Descriptor Shell Scripting
Fildesh is a domain-specific language to create nonlinear pipelines between programs.
It can create pipes between programs with multiple inputs
([eg/sum/add_test.fildesh](eg/sum/add_test.fildesh))
and multiple outputs
([eg/sum/dag.fildesh](eg/sum/dag.fildesh))
that depend on each other
([eg/sum/cycle.fildesh](eg/sum/cycle.fildesh)).
When the dataflow graph is not a tree,
the `elastic` builtin should be used to avoid deadlocks in the undirected cycles.

## Quick Start
### Docker
```shell
# Run script given as command-line args.
docker run -q --rm -a STDOUT ghcr.io/fildesh/fildesh:latest -- \
  '|< splice / "Hello world!\n" /' \
  '|> stdout'

# Run script given on standard input.
docker run -q --rm -i ghcr.io/fildesh/fildesh:latest - < example/hello.fildesh
```

### CMake
```shell
# Run script after compiling fildesh in ./bld/src/bin/.
cmake -B bld
cmake --build bld --target fildesh
./bld/src/bin/fildesh example/hello.fildesh

# Run script after compiling fildesh with aggressive development presets.
make run 'ARGS=example/hello.fildesh'
```

### Bazel
```shell
# Run script after compiling fildesh in ./bazel-bin/src/bin/.
bazel run //:fildesh -- $PWD/example/hello.fildesh
```

## Motivating Example
Consider writing an end-to-end test that checks whether a program gives an expected result for a particular input.
In order to compare the expected and actual results, at least one of them is typically stored as a temporary string or file.
With Fildesh, we can keep both in pipes.

The following example ([eg/sum/add_test.fildesh](eg/sum/add_test.fildesh)) shows how to test a program `add` that calculates the sum of numbers on each input line.
First, the script constructs an pipe named `expect` that contains the expected result.
Next, three input lines are constructed and stored in their own pipes named `input_line_1`, `input_linee_2`, and `input_line_3`.
Finally, those lines are spliced together, piped to `add`, and the result is compared against the expected one.
```shell
# Expected result sums on their own lines: 55, 0, and 5050.
splice -o $(OF expect) / "55\n" "0\n" "5050\n" /

# First test input line: Integers from 1 to 10.
|< seq 1 10
|> replace_string -o $(OF input_line_1) "\n" " "

# Second test input line: Nothing.
splice -o $(OF input_line_2) / "" /

# Third test input line: Integers from 1 to 100.
|< seq 1 100
|> replace_string -o $(OF input_line_3) "\n" " "

# Combine the test input lines.
|< splice $(XF input_line_1) / "\n" $(XA input_line_2) "\n" / $(XF input_line_3)
# System under test: Program that sums each line of input numbers.
|- add
# Compare the expected and actual results,
# and exit with a nonzero status if they differ.
|- cmp $(XF expect) -
# Show the diff on stdout if there is any.
|> stdout

$(barrier)
# Indicate success when all previous commands have finished successfully.
|< splice / "Success: `add` produced expected result.\n" /
|> stdout
```
