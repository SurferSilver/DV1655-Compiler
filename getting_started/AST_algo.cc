/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Includes and namespace declarations */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

#include "Node.h"
#include "SymbolTable.h"

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Visitor base class */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

class Visitor {
public:
    virtual ~Visitor() {}
    virtual void visit(Node* node, Node* parent = nullptr) = 0;
};

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Symbol Table Visitor */
/* Builds hierarchical scope tree */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

class SymbolTableVisitor : public Visitor {
private:
    Scope* globalScope;
    Scope* currentScope;
    int currentDepth;
    int scopeCounter;
    
public:
    SymbolTableVisitor() : currentDepth(0), scopeCounter(0) {
        globalScope = new Scope(scopeCounter++, currentDepth, "Global", nullptr);
        currentScope = globalScope;
    }
    
    ~SymbolTableVisitor() {
    }
    
    void visit(Node* node, Node* parent = nullptr) override {
        if (!node) return;
        
        if (node->type == "Class") {
            string className = node->value;
            if (!className.empty()) {
                currentScope->addSymbol(className, className, SymbolKind::CLASS);
            }
            string scopeName = "Class:" + className;
            currentDepth++;
            Scope* newScope = new Scope(scopeCounter++, currentDepth, scopeName, currentScope);
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
            currentDepth++;
            Scope* newScope = new Scope(scopeCounter++, currentDepth, scopeName, currentScope);
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
            string scopeName = "Method:main";
            currentDepth++;
            Scope* newScope = new Scope(scopeCounter++, currentDepth, scopeName, currentScope);
            currentScope = newScope;
        }
        else if (node->type == "BlockContent") {
            if (parent && (parent->type == "Method" || parent->type == "MainEntry")) {
            } else {
                string scopeName = "BlockContent";
                if (parent && parent->type == "ForStatement") {
                    scopeName = "ForContent";
                } else if (parent && parent->type == "IfStatement") {
                    scopeName = "IfContent";
                }
                currentDepth++;
                Scope* newScope = new Scope(scopeCounter++, currentDepth, scopeName, currentScope);
                currentScope = newScope;
            }
        }
        if (node->type == "Var") {
            addVariable(node);
        }
        for (auto child : node->children) {
            visit(child, node);
        }
        bool createdScope = (node->type == "Class" || node->type == "Method" || node->type == "MainEntry");
        if (node->type == "BlockContent" && !(parent && (parent->type == "Method" || parent->type == "MainEntry"))) {
            createdScope = true;
        }
        if (createdScope && currentScope->parent) {
            currentScope = currentScope->parent;
            currentDepth--;
        }
    }
    
private:
    void addVariable(Node* varNode) {
        if (!varNode || varNode->type != "Var") return;
        
        string type = "unknown";
        string name = "unknown";
        
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
        generateSymbolTableDotFile(globalScope);
    }
};

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Main function */
/* Reads tree.dot and builds symbol table */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

int main() {
    Node* root = parseTreeDot("tree.dot");
    if (!root) {
        cerr << "Error: Could not parse tree.dot" << endl;
        return 1;
    }
    
    SymbolTableVisitor visitor;
    visitor.visit(root);
    visitor.generateSymbolTableDot();
    return 0;
}
