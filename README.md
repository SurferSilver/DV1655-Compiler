# DV1655 - Compiler Course

A personal repository for the Compiler course assignments at BTH. This project implements a compiler for the CPM (C plus minus) language with support for lexical analysis, syntax parsing, and semantic validation.

## Repository Layout

The project is organized using **Git branches** to separate different phases of compiler development:

- **`main`** - Main branch (will be the final version when this project is finished)
- **`Assignment 1`** - Stage 1, Lexer and Parser
- **`Assignment 2`** - Stage 2, Semantic Analysis
- **`Assignment 3`** - Stage 3, Code Generation


## Getting Started

1. **Clone the repository:**
   ```bash
   git clone <repo-url>
   cd DV1655
   ```

2. **Switch to the desired branch:**
   ```bash
   git checkout getting_started
   ```

3. **Build and test:**
   ```bash
   make clean
   make
   make test-XXX
   ```

## Prerequisites

- GCC/G++ compiler (C++14 support)
- Flex lexical analyzer generator
- Bison parser generator
- Make build system

## Instructions
For branch-specific instructions, see the README in each branch.

## Future improvements
There is currently no future support if the assignments would change. The CPM language is specific for this project and any changes to this would break this solution.
