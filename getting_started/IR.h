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
        //current can be null if we call from global scope
        if (!current) return;
        current->addInstr(instr);
    }

    void visitClassList(Node* node) {
        //check for classes in classlist
        for (auto cls : node->children)
            //check for contentblock in each class           
            for (auto content : cls->children)
                //check for methods in contentblock    
                for (auto member : content->children)
                    if (member->type == "Method"){
                        current = cfg.newBlock(member->value);
                        // children: optional ParameterList, Type, BlockContent — skip Type/ParameterList
                        for (auto child : member->children)
                            if (child->type == "BlockContent") visitStatement(child);
                        }   
    }

    void visitMainEntry(Node* node) {
        //create entry in cfg
        current = cfg.newBlock("main");
        cfg.entry = current;
        //find blockcontent
        for (auto child : node->children)
            if (child->type == "BlockContent") visitStatement(child);
    }

    void visitStatement(Node* node) {
        if (!node) return;
        //loop through potential blockcontent
        if      (node->type == "BlockContent")    { for (auto c : node->children) visitStatement(c); }
        //the other cases
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
        //volatile case is not here it's inside the var node
        for (auto child : node->children)
            if (child->type == "Var") {
                //2 cases, implicit or assignm inside
                string varName;
                for (auto vchild : child->children)
                    if (vchild->type == "Id") varName = vchild->value;
                emit(TACInstr(TACOp::ASSIGN, varName, "0"));    
            }
            else if (child->type == "AssignStatement") {
                visitAssign(child);
            }
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
        //evaluate rhs first because if TAC instructions
        string val = visitExpr(rhs);
        if (lhs->type == "IndexExpression") {
            auto iit = lhs->children.begin();
            string arr = visitExpr(*iit++);
            string idx = visitExpr(*iit);
            //finns problem med typ list[i]:= val om val påverkar i
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
        auto it = node->children.begin();
        Node* condWrapper = *it++;
        Node* thenWrapper = (it != node->children.end()) ? *it++ : nullptr;
        Node* elseWrapper = (it != node->children.end()) ? *it   : nullptr;

        //find the conditions inside potential wrapper nodes
        Node* condExpr = (condWrapper && !condWrapper->children.empty()) ? condWrapper->children.front() : nullptr;
        Node* thenStmt = (thenWrapper && !thenWrapper->children.empty()) ? thenWrapper->children.front() : nullptr;
        Node* elseStmt = (elseWrapper && !elseWrapper->children.empty()) ? elseWrapper->children.front() : nullptr;

        string condVal    = visitExpr(condExpr);
        string labelFalse = newLabel("if_false");
        string labelEnd   = newLabel("if_end");
        string labelTrue  = newLabel("if_true");

        BasicBlock* condBB  = current;
        BasicBlock* trueBB  = cfg.newBlock();
        BasicBlock* falseBB = cfg.newBlock(labelFalse);
        BasicBlock* endBB   = cfg.newBlock(labelEnd);

        //create the conditional jump
        emit(TACInstr(TACOp::JUMP_IFNOT, labelFalse, condVal));
        cfg.addEdge(condBB, trueBB);
        cfg.addEdge(condBB, falseBB);

        //the if true case
        current = trueBB;
        if (thenStmt) visitStatement(thenStmt);
        emit(TACInstr(TACOp::JUMP, labelEnd));
        cfg.addEdge(trueBB, endBB);

        //the if false case, if not else then just jump to end
        current = falseBB;
        if (elseStmt) visitStatement(elseStmt);
        emit(TACInstr(TACOp::JUMP, labelEnd));
        cfg.addEdge(falseBB, endBB);

        //point back to the end, other code can continue
        current = endBB;
    }

    void visitFor(Node* node) {
        auto it = node->children.begin();

        //find the conditions inside potential wrapper nodes
        Node* startWrapper = (it != node->children.end()) ? *it++ : nullptr;
        Node* endWrapper   = (it != node->children.end()) ? *it++ : nullptr;
        Node* stepWrapper  = (it != node->children.end()) ? *it++ : nullptr;
        Node* body         = (it != node->children.end()) ? *it   : nullptr;
        Node* initStmt = (startWrapper && !startWrapper->children.empty()) ? startWrapper->children.front() : nullptr;
        Node* condExpr = (endWrapper   && !endWrapper->children.empty())   ? endWrapper->children.front()   : nullptr;

        string labelCond = newLabel("for_cond");
        string labelBody = newLabel("for_body");
        string labelEnd  = newLabel("for_end");

        //init and then jump to init
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
        if (node->type == "Boolean") return node->value;

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

        if (node->type == "NotExpression") {
            string a = visitExpr(node->children.front());
            string t = newTemp();
            emit(TACInstr(TACOp::NOT, t, a));
            return t;
        }

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
            auto it = node->children.begin();
            string elemType = (*it)->value; ++it;
            Node* listContent = (it != node->children.end()) ? *it : nullptr;
            string t = newTemp();
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

        //recurse into single child
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
        // Skip orphan empty blocks (no instructions and no edges)
        if (block->instrs.empty() && block->successors.empty() && block->predecessors.empty())
            continue;

        // Build the label: block name on top, then one row per TAC instruction
        string title = block->label.empty() ? "B" + to_string(block->id) : block->label;
        out << "    b" << block->id << " [label=\"{" << title;
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
