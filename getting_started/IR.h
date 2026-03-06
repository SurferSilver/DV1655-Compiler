#ifndef IR_H
#define IR_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include "Node.h"

using namespace std;

// TODO: TAC instruction struct
// WHY: Three-address code is the standard low-level IR format. Each instruction has
//      at most one operator and three operands (e.g. t1 = a + b). This makes
//      subsequent code generation trivial — each TAC maps to 1-3 bytecode ops.

// TODO: BasicBlock class
// WHY: A basic block is a straight-line sequence of TAC with no branches in or out
//      (except at the end). Grouping TAC into blocks lets us model control flow
//      (if/for) as edges between blocks in the CFG rather than inline jumps.

// TODO: CFG class
// WHY: The Control-Flow Graph connects basic blocks with directed edges representing
//      possible execution paths. The assignment requires a .dot visualisation of it,
//      and the code generator traverses it to emit instructions in the right order.

// TODO: IRGenerator - traverses AST and produces TAC / basic blocks / CFG
// WHY: The AST carries all the semantic info (types, structure) from earlier passes.
//      Walking it top-down lets us emit TAC for expressions and split on control-flow
//      statements (if/for/return) to start new basic blocks automatically.

// TODO: Generate .dot file for CFG visualization
// WHY: Explicitly required by the assignment. A .dot file lets us render the CFG as
//      a PDF (same as the AST) so the grader can visually verify the IR is correct.

#endif
