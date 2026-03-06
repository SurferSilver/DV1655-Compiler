#ifndef CODEGEN_H
#define CODEGEN_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "IR.h"

using namespace std;

// TODO: BytecodeGenerator - traverses CFG and emits stack-based bytecode to a text file
// WHY: The CFG gives us the correct order to visit blocks and the edges tell us where
//      to insert jump instructions. Each TAC instruction maps directly to a small
//      sequence of stack ops (PUSH / binary-op / STORE etc.), so visiting the CFG
//      is all that is needed to produce a complete, executable bytecode file.
//      Writing to a text file (one opcode + operands per line) means the interpreter
//      can simply read it line-by-line — no binary serialisation needed.

#endif
