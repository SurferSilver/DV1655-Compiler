# DV1655 - Compiler

A compiler implementation for the CPM (Compiler Programming Model) language, featuring lexical analysis, syntax parsing, and semantic validation.

## Prerequisites

You'll need the following tools installed:
- `g++` - C++ compiler (version supporting C++14 or later)
- `flex` - Lexical analyzer generator
- `bison` - Parser generator
- `make` - Build automation tool

On Ubuntu/Debian:
```bash
sudo apt-get install build-essential flex bison
```

On macOS:
```bash
brew install flex bison
```

## Project Structure

- **`grammar.g`** - CPM language grammar specification
- **`lexer.flex`** - Flex lexical analyzer definition
- **`parser.yy`** - Bison parser rules
- **`main.cc`** - Main compiler driver
- **`Node.h`** - AST node definitions
- **`test_files/`** - Test cases organized by type:
  - `valid/` - Valid CPM programs
  - `syntax_errors/` - Programs with syntax errors
  - `semantic_errors/` - Programs with semantic errors
  - `lexical_errors/` - Programs with lexical errors
  - `tree_validation/` - Tree structure validation tests

## Building

### Clean and build from scratch:
```bash
make clean
make
```

This will:
1. Generate `lex.yy.c` from `lexer.flex` using flex
2. Generate `parser.tab.cc` and `parser.tab.h` from `parser.yy` using bison
3. Compile and link all sources into the `compiler` executable

## Running the Compiler

### Compile a single file:
```bash
./compiler path/to/file.cpm
```

### Compile a test file:
```bash
make test
```
(Compiles `test.cpm` in the current directory)

### Compile specific valid test case:
```bash
make compile-valid test=1
```
(Compiles `test_files/valid/test1.cpm`, where test number can be 1-8)

## Testing

### Run all valid file tests:
```bash
make test-valid
```
Tests all files in `test_files/valid/`. Should compile without errors.

### Run syntax error tests:
```bash
make test-syntax-errors
```
Tests all files in `test_files/syntax_errors/`. Should fail during parsing.

### Run semantic error tests:
```bash
make test-semantic-errors
```
Tests all files in `test_files/semantic_errors/`. Should fail during semantic analysis.

### Run lexical error tests:
```bash
make test-lexical-errors
```
Tests all files in `test_files/lexical_errors/`. Should fail during lexical analysis.

### Run tree validation tests:
```bash
make test-tree-validation
```
Tests operator precedence and parse tree structure.

### Run all tests:
```bash
make test-all
```
Runs all test suites in sequence.

## Visualization

### Generate parse tree visualization (PDF):
```bash
make tree
```
Generates `tree.pdf` from `tree.dot` (requires Graphviz `dot` command).

## Cleaning Up

Remove all generated files and executables:
```bash
make clean
``` 