/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Symbol Table - Shared definitions for AST and Semantic Analysis */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include <string>

using namespace std;

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Symbol types and structures */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

enum class SymbolKind { CLASS, METHOD, VARIABLE, PARAMETER };

struct MethodInfo {
    string returnType;
    vector<pair<string, string>> parameters;  // (name, type)
};

struct Symbol {
    string name;
    string type;
    SymbolKind kind;
    MethodInfo* methodInfo;  // Only for methods
    
    Symbol(string n, string t, SymbolKind k) : name(n), type(t), kind(k), methodInfo(nullptr) {}
    
    string kindToString() const {
        switch(kind) {
            case SymbolKind::CLASS: return "class";
            case SymbolKind::METHOD: return "method";
            case SymbolKind::VARIABLE: return "variable";
            case SymbolKind::PARAMETER: return "parameter";
            default: return "unknown";
        }
    }
};

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Scope class */
/* Hierarchical scope with pointer to parent */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

class Scope {
public:
    int id;
    int depth;
    string name;
    string className;  // Track which class this scope belongs to
    Scope* parent;
    vector<Scope*> children;
    map<string, Symbol> symbols;
    
    Scope(int scopeId, int scopeDepth, string scopeName, Scope* parentScope = nullptr)
        : id(scopeId), depth(scopeDepth), name(scopeName), parent(parentScope) {
        if (parent) {
            parent->children.push_back(this);
            className = parent->className;
        }
    }
    
    void addSymbol(const string& name, const string& type, SymbolKind kind) {
        symbols.emplace(name, Symbol(name, type, kind));
    }
    
    void addMethodSymbol(const string& name, const string& returnType, const vector<pair<string, string>>& params) {
        Symbol sym(name, returnType, SymbolKind::METHOD);
        sym.methodInfo = new MethodInfo{returnType, params};
        symbols.emplace(name, sym);
    }
    
    Symbol* lookup(const string& name) {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            return &it->second;
        }
        if (parent) {
            return parent->lookup(name);
        }
        return nullptr;
    }
    
    Symbol* lookupLocal(const string& name) {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    bool existsInCurrentScope(const string& name) {
        return symbols.find(name) != symbols.end();
    }
    
    // Print this scope's symbols
    void print(int indent = 0) {
        string pad(indent * 2, ' ');
        cout << pad << "┌─ Scope " << depth << ": " << name;
        if (parent) {
            cout << " (parent: " << parent->name << ")";
        }
        cout << endl;
        
        if (symbols.empty()) {
            cout << pad << "│  (no symbols)" << endl;
        } else {
            for (auto& [symName, sym] : symbols) {
                cout << pad << "│  [" << sym.kindToString() << "] " << sym.name << " : " << sym.type << endl;
            }
        }
        
        for (auto child : children) {
            child->print(indent + 1);
        }
        cout << pad << "└─" << endl;
    }
};

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* DOT file generation utilities */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

inline void generateScopeDotNode(ofstream& out, Scope* scope) {
    out << "    scope_" << scope->id << " [label=<";
    out << "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\">";
    out << "<TR><TD COLSPAN=\"3\"><B>" << scope->name << " (Scope " << scope->depth << ")</B></TD></TR>";
    
    if (!scope->symbols.empty()) {
        out << "<TR><TD><B>Kind</B></TD><TD><B>Name</B></TD><TD><B>Type</B></TD></TR>";
        for (auto& [name, sym] : scope->symbols) {
            out << "<TR><TD>" << sym.kindToString() << "</TD><TD>" << sym.name << "</TD><TD>" << sym.type << "</TD></TR>";
        }
    } else {
        out << "<TR><TD COLSPAN=\"3\">(empty)</TD></TR>";
    }
    
    out << "</TABLE>>];" << endl;
    
    for (auto child : scope->children) {
        generateScopeDotNode(out, child);
    }
}

inline void generateScopeDotEdges(ofstream& out, Scope* scope) {
    for (auto child : scope->children) {
        out << "    scope_" << scope->id << " -> scope_" << child->id << ";" << endl;
        generateScopeDotEdges(out, child);
    }
}

inline void generateSymbolTableDotFile(Scope* globalScope, const string& filename = "symbol_table.dot") {
    ofstream dotOut(filename);
    if (!dotOut.is_open()) {
        cerr << "Error: Could not create " << filename << endl;
        return;
    }
    
    dotOut << "digraph ScopeTree {" << endl;
    dotOut << "    rankdir=TB;" << endl;
    dotOut << "    node [shape=plaintext, fontname=\"Courier\"];" << endl;
    dotOut << endl;
    
    generateScopeDotNode(dotOut, globalScope);
    generateScopeDotEdges(dotOut, globalScope);
    
    dotOut << "}" << endl;
    dotOut.close();
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Tree Parser - Builds AST from tree.dot */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

inline Node* parseTreeDot(const string& filename) {
    ifstream dotFile(filename);
    if (!dotFile.is_open()) {
        cerr << "Error: Could not open " << filename << endl;
        return nullptr;
    }
    
    string line;
    map<string, Node*> nodeMap;
    
    while (getline(dotFile, line)) {
        if (line.empty() || line.find("digraph") != string::npos || 
            line.find("}") == 0) continue;
        
        if (line.find("[label=") != string::npos) {
            size_t nodeIdStart = line.find("n");
            size_t nodeIdEnd = line.find(" ");
            size_t labelStart = line.find("\"") + 1;
            size_t labelEnd = line.find("\"", labelStart);
            
            if (nodeIdStart != string::npos && labelStart < labelEnd) {
                string nodeId = line.substr(nodeIdStart, nodeIdEnd - nodeIdStart);
                string label = line.substr(labelStart, labelEnd - labelStart);
                
                // Parse line number from separate lineno attribute (format: lineno=X)
                int lineNo = 0;
                size_t linenoPos = line.find("lineno=");
                if (linenoPos != string::npos) {
                    try {
                        size_t numStart = linenoPos + 7;  // length of "lineno="
                        size_t numEnd = line.find_first_of("];, ", numStart);
                        lineNo = stoi(line.substr(numStart, numEnd - numStart));
                    } catch (...) {
                        lineNo = 0;
                    }
                }
                
                size_t colonPos = label.find(":");
                string type = (colonPos != string::npos) 
                    ? label.substr(0, colonPos) 
                    : label;
                string value = (colonPos != string::npos) 
                    ? label.substr(colonPos + 1) 
                    : "";
                
                Node* node = new Node(type, value, lineNo);
                nodeMap[nodeId] = node;
            }
        }
        else if (line.find("->") != string::npos) {
            size_t fromStart = line.find("n");
            size_t fromEnd = line.find(" ");
            size_t toStart = line.rfind("n");
            
            if (fromStart != string::npos && toStart != string::npos && fromEnd != string::npos) {
                string fromId = line.substr(fromStart, fromEnd - fromStart);
                string toId = line.substr(toStart);
                while (!toId.empty() && (toId.back() == ' ' || toId.back() == '\n' || toId.back() == '\r')) {
                    toId.pop_back();
                }
                
                if (nodeMap.count(fromId) && nodeMap.count(toId)) {
                    nodeMap[fromId]->children.push_back(nodeMap[toId]);
                }
            }
        }
    }
    
    dotFile.close();
    
    // Find root
    Node* root = nullptr;
    for (auto& [id, node] : nodeMap) {
        if (node->type == "Program") {
            root = node;
            break;
        }
    }
    
    if (!root && !nodeMap.empty()) {
        root = nodeMap.begin()->second;
    }
    
    return root;
}

#endif // SYMBOL_TABLE_H
