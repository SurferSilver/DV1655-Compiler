#ifndef IR_H
#define IR_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include "Node.h"

using namespace std;

enum class TACOp {
    ASSIGN,
    ADD,
    SUB,
    MUL,
    DIV,
    EXP,
    NEG,
    NOT,
    EQ,
    NEQ,
    LT,
    GT,
    LEQ,
    GEQ,
    AND,
    OR,
    JUMP,
    JUMP_IF,
    JUMP_IFNOT,
    LABEL,
    PARAM,
    CALL,
    RETURN,
    PRINT,
    ARRAY_LOAD,
    ARRAY_STORE,
    READ,
    LENGTH,
    NEW_ARRAY,
};

/*--------------------------------------------------------------------------------------------------------------------------------*/
// Three-Address Code Instruction
/*--------------------------------------------------------------------------------------------------------------------------------*/

struct TACInstr {
    TACOp  op;
    string result;
    string arg1;
    string arg2;

    TACInstr(TACOp o, string r = "", string a1 = "", string a2 = "")
        : op(o), result(r), arg1(a1), arg2(a2) {}

    string toString() const {
        switch (op) {
            case TACOp::ASSIGN:      return result + " = " + arg1;
            case TACOp::ADD:         return result + " = " + arg1 + " + " + arg2;
            case TACOp::SUB:         return result + " = " + arg1 + " - " + arg2;
            case TACOp::MUL:         return result + " = " + arg1 + " * " + arg2;
            case TACOp::DIV:         return result + " = " + arg1 + " / " + arg2;
            case TACOp::EXP:         return result + " = " + arg1 + " ^ " + arg2;
            case TACOp::NEG:         return result + " = -" + arg1;
            case TACOp::NOT:         return result + " = !" + arg1;
            case TACOp::EQ:          return result + " = " + arg1 + " == " + arg2;
            case TACOp::NEQ:         return result + " = " + arg1 + " != " + arg2;
            case TACOp::LT:          return result + " = " + arg1 + " < " + arg2;
            case TACOp::GT:          return result + " = " + arg1 + " > " + arg2;
            case TACOp::LEQ:         return result + " = " + arg1 + " <= " + arg2;
            case TACOp::GEQ:         return result + " = " + arg1 + " >= " + arg2;
            case TACOp::AND:         return result + " = " + arg1 + " && " + arg2;
            case TACOp::OR:          return result + " = " + arg1 + " || " + arg2;
            case TACOp::JUMP:        return "goto " + result;
            case TACOp::JUMP_IF:     return "if " + arg1 + " goto " + result;
            case TACOp::JUMP_IFNOT:  return "ifnot " + arg1 + " goto " + result;
            case TACOp::LABEL:       return result + ":";
            case TACOp::PARAM:       return "param " + arg1;
            case TACOp::CALL:        return result + " = call " + arg1 + " " + arg2;
            case TACOp::RETURN:      return arg1.empty() ? "return" : "return " + arg1;
            case TACOp::PRINT:       return "print " + arg1;
            case TACOp::ARRAY_LOAD:  return result + " = " + arg1 + "[" + arg2 + "]";
            case TACOp::ARRAY_STORE: return arg1 + "[" + arg2 + "] = " + result;
            case TACOp::READ:        return "read " + result;
            case TACOp::LENGTH:      return result + " = " + arg1 + ".length";
            case TACOp::NEW_ARRAY:   return result + " = new_array " + arg1 + " [" + arg2 + "]";
            default:                 return "unknown";
        }
    }
};

/*--------------------------------------------------------------------------------------------------------------------------------*/
// Basic Block
/*--------------------------------------------------------------------------------------------------------------------------------*/

struct BasicBlock {
    int id;
    string label;
    vector<TACInstr> instrs;
    vector<BasicBlock*> successors;
    vector<BasicBlock*> predecessors;

    BasicBlock(int id, string label = "")
        : id(id), label(label) {}

    void addInstr(const TACInstr& instr) {
        instrs.push_back(instr);
    }
};

struct CFG {
    vector<BasicBlock*> blocks;
    BasicBlock* entry = nullptr;
    int nextId = 0;

    BasicBlock* newBlock(string label = "") {
        if (label.empty()) label = "B" + to_string(nextId);
        BasicBlock* b = new BasicBlock(nextId++, label);
        blocks.push_back(b);
        return b;
    }

    void addEdge(BasicBlock* from, BasicBlock* to) {
        from->successors.push_back(to);
        to->predecessors.push_back(from);
    }

    BasicBlock* getBlockByLabel(const string& label) {
        for (auto b : blocks)
            if (b->label == label) return b;
        return nullptr;
    }

    ~CFG() {
        for (auto b : blocks) delete b;
    }
};

/*--------------------------------------------------------------------------------------------------------------------------------*/
// IR Generator - walks the AST and produces TAC + CFG
/*--------------------------------------------------------------------------------------------------------------------------------*/

class IRGenerator {
public:
    CFG cfg;

    void generate(Node* root) {
        if (!root) return;
        for (auto child : root->children) {
            if (child->type == "MainEntry")
                visitMainEntry(child);
            else if (child->type == "ClassList")
                visitClassList(child);
        }
    }

private:
    BasicBlock* current = nullptr;
    int tempCount = 0;
    int labelCount = 0;
    vector<string> breakLabelStack;

    string newTemp() { return "t" + to_string(tempCount++); }
    string newLabel(const string& prefix = "L") { return prefix + to_string(labelCount++); }

    void emit(const TACInstr& instr) {
        if (!current) return;
        current->addInstr(instr);
    }

    void visitNode(Node* node) {
        if (!node) return;
        if (node->type == "MainEntry")   visitMainEntry(node);
        else if (node->type == "Method") visitMethod(node);
    }

    void visitClassList(Node* node) {
        for (auto cls : node->children)           // Class nodes
            for (auto content : cls->children)    // ClassContent
                for (auto member : content->children)
                    if (member->type == "Method") visitMethod(member);
    }

    void visitMainEntry(Node* node) {
        current = cfg.newBlock("main");
        cfg.entry = current;
        // children: Type, BlockContent — skip the Type node
        for (auto child : node->children)
            if (child->type != "Type") visitStatement(child);
    }

    void visitMethod(Node* node) {
        current = cfg.newBlock(node->value);
        // children: optional ParameterList, Type, BlockContent — skip Type/ParameterList
        for (auto child : node->children)
            if (child->type != "Type" && child->type != "ParameterList") visitStatement(child);
    }

    void visitStatement(Node* node) {
        if (!node) return;
        if      (node->type == "BlockContent")    { for (auto c : node->children) visitStatement(c); }
        else if (node->type == "VarDeclaration")  visitVarDeclaration(node);
        else if (node->type == "AssignStatement") visitAssign(node);
        else if (node->type == "PrintStatement")  visitPrint(node);
        else if (node->type == "ReturnStatement") visitReturn(node);
        else if (node->type == "IfStatement")     visitIf(node);
        else if (node->type == "ForStatement")    visitFor(node);
        else if (node->type == "ExpressionStatement") {
            if (!node->children.empty()) visitExpr(node->children.front());
        }
        else if (node->type == "ReadStatement")  visitRead(node);
        else if (node->type == "BreakStatement") visitBreak();
    }

    void visitVarDeclaration(Node* node) {
        // child is a Var node, which may be an AssignStatement wrapping a Var
        for (auto child : node->children)
            visitStatement(child);
    }

    void visitRead(Node* node) {
        if (node->children.empty()) return;
        string dest = visitExpr(node->children.front());
        emit(TACInstr(TACOp::READ, dest));
    }

    void visitBreak() {
        if (!breakLabelStack.empty())
            emit(TACInstr(TACOp::JUMP, breakLabelStack.back()));
    }

    void visitAssign(Node* node) {
        auto it = node->children.begin();
        Node* lhs = *it++;
        Node* rhs = *it;
        string val = visitExpr(rhs);
        // Array store: list[i] := val
        if (lhs->type == "IndexExpression") {
            auto iit = lhs->children.begin();
            string arr = visitExpr(*iit++);
            string idx = visitExpr(*iit);
            emit(TACInstr(TACOp::ARRAY_STORE, val, arr, idx));
        } else {
            string dest = lhsName(lhs);
            emit(TACInstr(TACOp::ASSIGN, dest, val));
        }
    }

    void visitPrint(Node* node) {
        string val = visitExpr(node->children.front());
        emit(TACInstr(TACOp::PRINT, "", val));
    }

    void visitReturn(Node* node) {
        if (!node->children.empty()) {
            string val = visitExpr(node->children.front());
            emit(TACInstr(TACOp::RETURN, "", val));
        } else {
            emit(TACInstr(TACOp::RETURN));
        }
    }

    void visitIf(Node* node) {
        // children: condition, then_branch, [else_branch]  (each a wrapper node)
        auto it = node->children.begin();
        Node* condWrapper = *it++;
        Node* thenWrapper = (it != node->children.end()) ? *it++ : nullptr;
        Node* elseWrapper = (it != node->children.end()) ? *it   : nullptr;

        Node* condExpr = (condWrapper && !condWrapper->children.empty()) ? condWrapper->children.front() : nullptr;
        Node* thenStmt = (thenWrapper && !thenWrapper->children.empty()) ? thenWrapper->children.front() : nullptr;
        Node* elseStmt = (elseWrapper && !elseWrapper->children.empty()) ? elseWrapper->children.front() : nullptr;

        string condVal    = visitExpr(condExpr);
        string labelFalse = newLabel("if_false");
        string labelEnd   = newLabel("if_end");

        BasicBlock* condBB  = current;
        BasicBlock* trueBB  = cfg.newBlock(newLabel("if_true"));
        BasicBlock* falseBB = cfg.newBlock(labelFalse);
        BasicBlock* endBB   = cfg.newBlock(labelEnd);

        emit(TACInstr(TACOp::JUMP_IFNOT, labelFalse, condVal));
        cfg.addEdge(condBB, trueBB);
        cfg.addEdge(condBB, falseBB);

        current = trueBB;
        if (thenStmt) visitStatement(thenStmt);
        emit(TACInstr(TACOp::JUMP, labelEnd));
        cfg.addEdge(trueBB, endBB);

        current = falseBB;
        if (elseStmt) visitStatement(elseStmt);
        emit(TACInstr(TACOp::JUMP, labelEnd));
        cfg.addEdge(falseBB, endBB);

        current = endBB;
    }

    void visitFor(Node* node) {
        // children: start_condition, end_condition, step, body
        // start_condition and end_condition each wrap one optional child
        // step wraps: lhs-expr, rhs-expr (the update assignment)
        auto it = node->children.begin();
        Node* startWrapper = (it != node->children.end()) ? *it++ : nullptr;
        Node* endWrapper   = (it != node->children.end()) ? *it++ : nullptr;
        Node* stepWrapper  = (it != node->children.end()) ? *it++ : nullptr;
        Node* body         = (it != node->children.end()) ? *it   : nullptr;

        Node* initStmt = (startWrapper && !startWrapper->children.empty()) ? startWrapper->children.front() : nullptr;
        Node* condExpr = (endWrapper   && !endWrapper->children.empty())   ? endWrapper->children.front()   : nullptr;

        string labelCond = newLabel("for_cond");
        string labelBody = newLabel("for_body");
        string labelEnd  = newLabel("for_end");

        BasicBlock* initBB = current;
        if (initStmt) visitStatement(initStmt);

        BasicBlock* condBB = cfg.newBlock(labelCond);
        cfg.addEdge(initBB, condBB);
        emit(TACInstr(TACOp::JUMP, labelCond));

        current = condBB;
        if (condExpr) {
            string condVal = visitExpr(condExpr);
            emit(TACInstr(TACOp::JUMP_IFNOT, labelEnd, condVal));
        }

        BasicBlock* bodyBB = cfg.newBlock(labelBody);
        BasicBlock* endBB  = cfg.newBlock(labelEnd);
        cfg.addEdge(condBB, bodyBB);
        cfg.addEdge(condBB, endBB);

        breakLabelStack.push_back(labelEnd);
        current = bodyBB;
        if (body) visitStatement(body);
        breakLabelStack.pop_back();

        // step: two children are lhs-expr and rhs-expr of the update assignment
        if (stepWrapper && stepWrapper->children.size() >= 2) {
            auto sit = stepWrapper->children.begin();
            Node* stepLhs = *sit++;
            Node* stepRhs = *sit;
            string val  = visitExpr(stepRhs);
            string dest = lhsName(stepLhs);
            emit(TACInstr(TACOp::ASSIGN, dest, val));
        }

        emit(TACInstr(TACOp::JUMP, labelCond));
        cfg.addEdge(bodyBB, condBB);

        current = endBB;
    }

    string visitExpr(Node* node) {
        if (!node) return "";

        if (node->type == "Id")      return node->value;
        if (node->type == "Int")     return node->value;
        if (node->type == "Float")   return node->value;
        if (node->type == "Boolean") return node->value; // parser emits "Boolean" with value "true"/"false"

        // Parser emits specific expression type names, not generic "BinOp"
        if (node->type == "AddExpression") return emitBinOp(node, TACOp::ADD);
        if (node->type == "SubExpression") return emitBinOp(node, TACOp::SUB);
        if (node->type == "MultExpression")return emitBinOp(node, TACOp::MUL);
        if (node->type == "DivExpression") return emitBinOp(node, TACOp::DIV);
        if (node->type == "ExpExpression") return emitBinOp(node, TACOp::EXP);
        if (node->type == "EqExpression")  return emitBinOp(node, TACOp::EQ);
        if (node->type == "NeqExpression") return emitBinOp(node, TACOp::NEQ);
        if (node->type == "LtExpression")  return emitBinOp(node, TACOp::LT);
        if (node->type == "GtExpression")  return emitBinOp(node, TACOp::GT);
        if (node->type == "LeqExpression") return emitBinOp(node, TACOp::LEQ);
        if (node->type == "GeqExpression") return emitBinOp(node, TACOp::GEQ);
        if (node->type == "AndExpression") return emitBinOp(node, TACOp::AND);
        if (node->type == "OrExpression")  return emitBinOp(node, TACOp::OR);

        // Parser emits "NotExpression", not "UnaryOp"
        if (node->type == "NotExpression") {
            string a = visitExpr(node->children.front());
            string t = newTemp();
            emit(TACInstr(TACOp::NOT, t, a));
            return t;
        }

        // obj.method(args) — FieldAccess child carries the object, node->value is empty
        if (node->type == "MethodCall") return visitMethodCall(node);

        if (node->type == "IndexExpression") {
            auto it = node->children.begin();
            string arr = visitExpr(*it++);
            string idx = visitExpr(*it);
            string t   = newTemp();
            emit(TACInstr(TACOp::ARRAY_LOAD, t, arr, idx));
            return t;
        }

        if (node->type == "LengthExpression") {
            string arr = visitExpr(node->children.front());
            string t = newTemp();
            emit(TACInstr(TACOp::LENGTH, t, arr));
            return t;
        }

        if (node->type == "ListExpression") {
            // children: Type, ListContent (which holds the element expressions)
            auto it = node->children.begin();
            string elemType = (*it)->value; ++it;
            Node* listContent = (it != node->children.end()) ? *it : nullptr;
            string t = newTemp();
            // Collect elements as a comma-separated string for the TAC operand
            string elems;
            if (listContent) {
                for (auto elem : listContent->children) {
                    if (!elems.empty()) elems += ",";
                    elems += visitExpr(elem);
                }
            }
            emit(TACInstr(TACOp::NEW_ARRAY, t, elemType, elems));
            return t;
        }

        // Fallback: recurse into single child (handles wrapper nodes)
        if (!node->children.empty())
            return visitExpr(node->children.front());

        return node->value;
    }

    string emitBinOp(Node* node, TACOp op) {
        auto it = node->children.begin();
        string a1 = visitExpr(*it++);
        string a2 = visitExpr(*it);
        string t  = newTemp();
        emit(TACInstr(op, t, a1, a2));
        return t;
    }

    string visitMethodCall(Node* node) {
        // children: [FieldAccess|Id], [ArgList]
        // FieldAccess carries the object; the method name is in its value or the Id value
        string methodName;
        vector<string> args;

        for (auto child : node->children) {
            if (child->type == "ArgList") {
                for (auto arg : child->children)
                    args.push_back(visitExpr(arg));
            } else if (child->type == "FieldAccess") {
                methodName = child->value;
            } else if (child->type == "Id") {
                methodName = child->value;
            }
        }

        for (auto& a : args)
            emit(TACInstr(TACOp::PARAM, "", a));

        string t = newTemp();
        emit(TACInstr(TACOp::CALL, t, methodName, to_string(args.size())));
        return t;
    }

    string lhsName(Node* lhs) {
        if (!lhs) return "_";
        if (lhs->type == "Id") return lhs->value;
        // Var node: children are [Volatile?], Id, Type — find the Id
        for (auto child : lhs->children)
            if (child->type == "Id") return child->value;
        return lhs->value;
    }

    TACOp opFromString(const string& op) {
        if (op == "+")  return TACOp::ADD;
        if (op == "-")  return TACOp::SUB;
        if (op == "*")  return TACOp::MUL;
        if (op == "/")  return TACOp::DIV;
        if (op == "^")  return TACOp::EXP;
        if (op == "==") return TACOp::EQ;
        if (op == "!=") return TACOp::NEQ;
        if (op == "<")  return TACOp::LT;
        if (op == ">")  return TACOp::GT;
        if (op == "<=") return TACOp::LEQ;
        if (op == ">=") return TACOp::GEQ;
        if (op == "&&") return TACOp::AND;
        if (op == "||") return TACOp::OR;
        return TACOp::ASSIGN;
    }
};

// TODO: Generate .dot file for CFG

inline void generateCFGDot(const CFG& cfg, const string& filename = "cfg.dot") {
    ofstream out(filename);

    out << "digraph CFG {\n";
    out << "    node [shape=record fontname=\"Courier\"];\n\n";

    for (auto block : cfg.blocks) {
        // Build the label: block name on top, then one row per TAC instruction
        out << "    b" << block->id << " [label=\"{" << block->label;
        for (auto& instr : block->instrs) {
            string s = instr.toString();
            // Escape characters that break DOT record labels
            string escaped;
            for (char c : s) {
                if (c == '"')       escaped += "\\\"";
                else if (c == '{')  escaped += "\\{";
                else if (c == '}')  escaped += "\\}";
                else if (c == '<')  escaped += "\\<";
                else if (c == '>')  escaped += "\\>";
                else if (c == '|')  escaped += "\\|";
                else                escaped += c;
            }
            out << " | " << escaped;
        }
        out << "}\"];\n";
    }

    out << "\n";

    // Edges
    for (auto block : cfg.blocks)
        for (auto succ : block->successors)
            out << "    b" << block->id << " -> b" << succ->id << ";\n";

    out << "}\n";
    out.close();

    printf("\nBuilt a CFG at %s. Use 'make cfg' to generate the pdf version.\n", filename.c_str());
}

#endif
