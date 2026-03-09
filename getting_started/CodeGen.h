#ifndef CODEGEN_H
#define CODEGEN_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include "IR.h"

using namespace std;

class BytecodeGenerator {
public:
	bool generate(const CFG& cfg, const string& filename = "bytecode.txt") {
		ofstream out(filename);
        //error handling
		if (!out.is_open()) {
			cerr << "Error: could not open bytecode output file '" << filename << "'" << endl;
			return false;
		}

		// Start execution from CFG entry to avoid accidental fallthrough
		if (cfg.entry) {
			out << "JUMP " << blockLabel(cfg.entry) << "\n";
		}

		for (auto* block : cfg.blocks) {
			if (!block) continue;
			out << blockLabel(block) << ":\n";

			for (const auto& instr : block->instrs) {
				emitInstr(instr, out);
			}
		}

        //program end
		out << "__program_end:\n";
		out << "HALT\n";
		out.close();
		return true;
	}

private:
	static bool isNumber(const string& s) {
		if (s.empty()) return false;
		char* end = nullptr;
		strtod(s.c_str(), &end);
		return end && *end == '\0';
	}

	static bool isBooleanLiteral(const string& s) {
		return s == "true" || s == "false";
	}

	static string blockLabel(const BasicBlock* block) {
		if (!block) return "";
		if (!block->label.empty()) return block->label;
		return "B" + to_string(block->id);
	}

	static void emitLoadOperand(const string& operand, ofstream& out) {
		if (operand.empty()) return;

		if (isBooleanLiteral(operand)) {
			out << "PUSH " << (operand == "true" ? "1" : "0") << "\n";
			return;
		}

		if (isNumber(operand)) {
			out << "PUSH " << operand << "\n";
			return;
		}

		out << "LOAD " << operand << "\n";
	}

	static void emitBinary(const TACInstr& instr, const string& opcode, ofstream& out) {
		emitLoadOperand(instr.arg1, out);
		emitLoadOperand(instr.arg2, out);
		out << opcode << "\n";
		if (!instr.result.empty()) out << "STORE " << instr.result << "\n";
	}

	static void emitInstr(const TACInstr& instr, ofstream& out) {
		switch (instr.op) {
			case TACOp::DECLARE:
				out << "DECLARE " << instr.result << "\n";
				break;

			case TACOp::ASSIGN:
				emitLoadOperand(instr.arg1, out);
				out << "STORE " << instr.result << "\n";
				break;

			case TACOp::ADD: emitBinary(instr, "ADD", out); break;
			case TACOp::SUB: emitBinary(instr, "SUB", out); break;
			case TACOp::MUL: emitBinary(instr, "MUL", out); break;
			case TACOp::DIV: emitBinary(instr, "DIV", out); break;
			case TACOp::EXP: emitBinary(instr, "EXP", out); break;
			case TACOp::EQ:  emitBinary(instr, "EQ", out);  break;
			case TACOp::NEQ: emitBinary(instr, "NEQ", out); break;
			case TACOp::LT:  emitBinary(instr, "LT", out);  break;
			case TACOp::GT:  emitBinary(instr, "GT", out);  break;
			case TACOp::LEQ: emitBinary(instr, "LEQ", out); break;
			case TACOp::GEQ: emitBinary(instr, "GEQ", out); break;
			case TACOp::AND: emitBinary(instr, "AND", out); break;
			case TACOp::OR:  emitBinary(instr, "OR", out);  break;

			case TACOp::NEG:
				emitLoadOperand(instr.arg1, out);
				out << "NEG\n";
				out << "STORE " << instr.result << "\n";
				break;

			case TACOp::NOT:
				emitLoadOperand(instr.arg1, out);
				out << "NOT\n";
				out << "STORE " << instr.result << "\n";
				break;

			case TACOp::JUMP:
				out << "JUMP " << instr.result << "\n";
				break;

			case TACOp::JUMP_IF:
				emitLoadOperand(instr.arg1, out);
				out << "JUMP_IF " << instr.result << "\n";
				break;

			case TACOp::JUMP_IFNOT:
				emitLoadOperand(instr.arg1, out);
				out << "JUMP_IFNOT " << instr.result << "\n";
				break;

			case TACOp::LABEL:
				out << instr.result << ":\n";
				break;

			case TACOp::PRINT:
				emitLoadOperand(instr.arg1, out);
				out << "PRINT\n";
				break;

			case TACOp::READ:
				out << "READ " << instr.result << "\n";
				break;

			case TACOp::RETURN:
				if (!instr.arg1.empty()) {
					emitLoadOperand(instr.arg1, out);
					out << "RETVAL\n";
				} else {
					out << "RET\n";
				}
				break;

			case TACOp::PARAM:
				emitLoadOperand(instr.arg1, out);
				out << "PARAM\n";
				break;

			case TACOp::CALL:
				out << "CALL " << instr.arg1 << " " << instr.arg2 << "\n";
				if (!instr.result.empty()) out << "STORE " << instr.result << "\n";
				break;

			case TACOp::GET_PARAM:
				out << "GET_PARAM " << instr.result << " " << instr.arg1 << "\n";
				break;

			// These TAC ops are not executable in the current stack VM version.
			case TACOp::ARRAY_LOAD:
			case TACOp::ARRAY_STORE:
			case TACOp::LENGTH:
			case TACOp::NEW_ARRAY:
				if (!instr.result.empty()) {
					out << "PUSH 0\n";
					out << "STORE " << instr.result << "\n";
				}
				break;
		}
	}
};

#endif
