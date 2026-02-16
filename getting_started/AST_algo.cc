#include <iostream>
#include <map>
#include <stack>
#include <fstream>
#include <vector>
#include "Node.h"

using namespace std;

// SEPARATE RECORD TYPES for Classes, Methods, and Variables

enum class SymbolKind { CLASS, METHOD, VARIABLE };

struct Symbol {
    string name;
    string type;
    SymbolKind kind;
    
    Symbol(string n, string t, SymbolKind k) : name(n), type(t), kind(k) {}
    
    string kindToString() const {
        switch(kind) {
            case SymbolKind::CLASS: return "class";
            case SymbolKind::METHOD: return "method";
            case SymbolKind::VARIABLE: return "variable";
            default: return "unknown";
        }
    }
};

// SCOPE - Hierarchical scope with pointer to parent

class Scope {
public:
    int id;
    string name;                      
    Scope* parent;                    
    vector<Scope*> children;          
    map<string, Symbol> symbols;      
    
    Scope(int scopeId, string scopeName, Scope* parentScope = nullptr) 
        : id(scopeId), name(scopeName), parent(parentScope) {
        if (parent) {
            parent->children.push_back(this);
        }
    }
    
    void addSymbol(const string& name, const string& type, SymbolKind kind) {
        symbols.emplace(name, Symbol(name, type, kind));
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
    
    bool existsInCurrentScope(const string& name) {
        return symbols.find(name) != symbols.end();
    }
    
    // Print this scope's symbols
    void print(int indent = 0) {
        string pad(indent * 2, ' ');
        cout << pad << "┌─ Scope " << id << ": " << name;
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

// VISITOR BASE CLASS

class Visitor {
public:
    virtual ~Visitor() {}
    virtual void visit(Node* node) = 0;
};

// SYMBOL TABLE VISITOR - Builds hierarchical scope tree

class SymbolTableVisitor : public Visitor {
private:
    Scope* globalScope;
    Scope* currentScope;
    int scopeCounter;
    
public:
    SymbolTableVisitor() : scopeCounter(0) {
        globalScope = new Scope(scopeCounter++, "Global", nullptr);
        currentScope = globalScope;
    }
    
    ~SymbolTableVisitor() {
    }
    
    void visit(Node* node) override {
        if (!node) return;
        
        // Track Classes as symbols AND create scope
        if (node->type == "Class") {
            string className = node->value;
            if (!className.empty()) {
                currentScope->addSymbol(className, className, SymbolKind::CLASS);
            }
            string scopeName = "Class:" + className;
            Scope* newScope = new Scope(scopeCounter++, scopeName, currentScope);
            currentScope = newScope;
        }
        else if (node->type == "Method") {
            string methodName = node->value;
            string returnType = "void";
            for (auto child : node->children) {
                if (child->type == "Type") {
                    returnType = child->value;
                    break;
                }
            }
            if (!methodName.empty()) {
                currentScope->addSymbol(methodName, returnType, SymbolKind::METHOD);
            }
            string scopeName = "Method:" + methodName;
            Scope* newScope = new Scope(scopeCounter++, scopeName, currentScope);
            currentScope = newScope;
        }
        else if (node->type == "MainEntry") {
            string returnType = "int";
            for (auto child : node->children) {
                if (child->type == "Type") {
                    returnType = child->value;
                    break;
                }
            }
            currentScope->addSymbol("main", returnType, SymbolKind::METHOD);
        }
        else if (node->type == "BlockContent") {
            Scope* newScope = new Scope(scopeCounter++, "BlockContent", currentScope);
            currentScope = newScope;
        }
        if (node->type == "Var") {
            addVariable(node);
        }
        // LEFT-TO-RIGHT traversal of all children
        for (auto child : node->children) {
            visit(child);
        }
        if ((node->type == "Class" || node->type == "Method" || 
             node->type == "BlockContent") 
            && currentScope->parent) {
            currentScope = currentScope->parent;
        }
    }
    
private:
    void addVariable(Node* varNode) {
        if (!varNode || varNode->type != "Var") return;
        
        string type = "unknown";
        string name = "unknown";
        
        // Find Type and Id in Var's children
        for (auto child : varNode->children) {
            if (child->type == "Type") {
                type = child->value;
            }
            if (child->type == "Id") {
                name = child->value;
            }
        }
        
        if (name != "unknown") {
            currentScope->addSymbol(name, type, SymbolKind::VARIABLE);
        }
    }
    
public:
    void generateSymbolTableDot() {
        ofstream dotOut("symbol_table.dot");
        if (!dotOut.is_open()) {
            cerr << "Error: Could not create symbol_table.dot" << endl;
            return;
        }
        
        dotOut << "digraph ScopeTree {" << endl;
        dotOut << "    rankdir=TB;" << endl;
        dotOut << "    node [shape=plaintext, fontname=\"Courier\"];" << endl;
        dotOut << endl;
        
        // Generate nodes for each scope
        generateScopeDotNode(dotOut, globalScope);
        
        // Generate edges (parent -> child relationships)
        generateScopeDotEdges(dotOut, globalScope);
        
        dotOut << "}" << endl;
        dotOut.close();
    }
    
private:
    void generateScopeDotNode(ofstream& out, Scope* scope) {
        out << "    scope_" << scope->id << " [label=<";
        out << "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\">";
        out << "<TR><TD COLSPAN=\"3\"><B>" << scope->name << " (Scope " << scope->id << ")</B></TD></TR>";
        
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
    
    void generateScopeDotEdges(ofstream& out, Scope* scope) {
        for (auto child : scope->children) {
            out << "    scope_" << scope->id << " -> scope_" << child->id << ";" << endl;
            generateScopeDotEdges(out, child);
        }
    }
};

int main() {
    ifstream dotFile("tree.dot");
    if (!dotFile.is_open()) {
        cerr << "Error: Could not open tree.dot file." << endl;
        return 1;
    }
    
    SymbolTableVisitor visitor;
    
    string line;
    map<string, Node*> nodeMap;  // Map node ids to Node objects
    int nextId = 0;
    
    while (getline(dotFile, line)) {
        // Skip empty lines and digraph declaration
        if (line.empty() || line.find("digraph") != string::npos || 
            line.find("}") == 0) continue;
        
        // Parse node declarations: nX [label="Type:Value"];
        if (line.find("[label=") != string::npos) {
            size_t nodeIdStart = line.find("n");
            size_t nodeIdEnd = line.find(" ");
            size_t labelStart = line.find("\"") + 1;
            size_t labelEnd = line.rfind("\"");
            
            if (nodeIdStart != string::npos && labelStart < labelEnd) {
                string nodeId = line.substr(nodeIdStart, nodeIdEnd - nodeIdStart);
                string label = line.substr(labelStart, labelEnd - labelStart);
                
                // Extract node number from nodeId (e.g., "n5" -> 5)
                int lineNo = 0;
                if (nodeId.length() > 1) {
                    lineNo = stoi(nodeId.substr(1));
                }
                
                // Parse label: "Type:Value"
                size_t colonPos = label.find(":");
                string type = (colonPos != string::npos) 
                    ? label.substr(0, colonPos) 
                    : label;
                string value = (colonPos != string::npos) 
                    ? label.substr(colonPos + 1) 
                    : "";
                
                // Create Node with proper line number
                Node* node = new Node(type, value, lineNo);
                nodeMap[nodeId] = node;
            }
        }
        // Parse edges: nX -> nY
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
    
    if (nodeMap.empty()) {
        cerr << "Error: No nodes found in tree.dot" << endl;
        return 1;
    }
    
    // Find root node (usually first one or Program node)
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
    
    if (!root) {
        cerr << "Error: Could not find root node" << endl;
        return 1;
    }
    
    visitor.visit(root);
    visitor.generateSymbolTableDot();
    return 0;
}
