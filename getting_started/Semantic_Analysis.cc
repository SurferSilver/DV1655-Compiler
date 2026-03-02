/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Semantic Analysis for C+- Compiler */
/* Part 2: Type Checking and Semantic Error Detection */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

#include <iostream>
#include <map>
#include <stack>
#include <fstream>
#include <vector>
#include <set>
#include "Node.h"

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
};

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Semantic Error Reporter */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

class SemanticError {
public:
    int line;
    string message;
    
    SemanticError(int l, const string& msg) : line(l), message(msg) {}
};

class ErrorReporter {
private:
    vector<SemanticError> errors;
    
public:
    void report(int line, const string& message) {
        errors.push_back(SemanticError(line, message));
        cerr << "Semantic Error (line " << line << "): " << message << endl;
    }
    
    bool hasErrors() const { return !errors.empty(); }
    
    int errorCount() const { return errors.size(); }
    
    void printSummary() {
        if (errors.empty()) {
            cout << "No semantic errors found." << endl;
        } else {
            cout << "\n=== Semantic Analysis Summary ===" << endl;
            cout << "Total errors: " << errors.size() << endl;
        }
    }
};

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Global Symbol Table for Classes */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

class GlobalSymbolTable {
public:
    map<string, Scope*> classScopes;
    set<string> validTypes;
    
    GlobalSymbolTable() {
        // Primitive types
        validTypes.insert("int");
        validTypes.insert("float");
        validTypes.insert("boolean");
        validTypes.insert("int[]");
        validTypes.insert("float[]");
        validTypes.insert("boolean[]");
        validTypes.insert("void");
    }
    
    // Helper to check if a type is an array type
    static bool isArrayType(const string& type) {
        return type.size() > 2 && type.substr(type.size() - 2) == "[]";
    }
    
    // Get element type from array type (e.g., "int[]" -> "int")
    static string getElementType(const string& arrayType) {
        if (isArrayType(arrayType)) {
            return arrayType.substr(0, arrayType.size() - 2);
        }
        return arrayType;
    }
    
    void addClass(const string& name, Scope* scope) {
        classScopes[name] = scope;
        validTypes.insert(name);
    }
    
    Scope* getClassScope(const string& name) {
        auto it = classScopes.find(name);
        return (it != classScopes.end()) ? it->second : nullptr;
    }
    
    bool isValidType(const string& type) {
        return validTypes.find(type) != validTypes.end();
    }
    
    Symbol* lookupMethodInClass(const string& className, const string& methodName) {
        Scope* classScope = getClassScope(className);
        if (classScope) {
            return classScope->lookupLocal(methodName);
        }
        return nullptr;
    }
};

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Semantic Analyzer */
/* Two-pass: 1) Build symbol table  2) Type check */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

class SemanticAnalyzer {
private:
    Scope* globalScope;
    Scope* currentScope;
    int currentDepth;
    int scopeCounter;
    ErrorReporter& errorReporter;
    GlobalSymbolTable globalTable;
    string currentClassName;
    string currentMethodReturnType;
    bool foundReturn;
    
public:
    SemanticAnalyzer(ErrorReporter& er) 
        : currentDepth(0), scopeCounter(0), errorReporter(er), foundReturn(false) {
        globalScope = new Scope(scopeCounter++, currentDepth, "Global", nullptr);
        currentScope = globalScope;
    }
    
    /* ========== PASS 1: Build Symbol Table ========== */
    
    void buildSymbolTable(Node* node, Node* parent = nullptr) {
        if (!node) return;
        
        // Collect class declarations first
        if (node->type == "Class") {
            string className = node->value;
            
            // Check for duplicate class
            if (globalTable.getClassScope(className)) {
                errorReporter.report(node->lineno, "Duplicate class declaration: '" + className + "'");
            }
            
            currentScope->addSymbol(className, className, SymbolKind::CLASS);
            currentDepth++;
            Scope* classScope = new Scope(scopeCounter++, currentDepth, "Class:" + className, currentScope);
            classScope->className = className;
            currentScope = classScope;
            currentClassName = className;
            
            globalTable.addClass(className, classScope);
        }
        else if (node->type == "Method") {
            string methodName = node->value;
            string returnType = "void";
            vector<pair<string, string>> params;
            
            // Get return type and parameters
            for (auto child : node->children) {
                if (child->type == "Type") {
                    returnType = child->value;
                }
                else if (child->type == "ParameterList") {
                    collectParameters(child, params);
                }
            }
            
            // Check for duplicate method in current scope
            if (currentScope->existsInCurrentScope(methodName)) {
                errorReporter.report(node->lineno, "Duplicate method declaration: '" + methodName + "'");
            }
            
            currentScope->addMethodSymbol(methodName, returnType, params);
            
            currentDepth++;
            Scope* methodScope = new Scope(scopeCounter++, currentDepth, "Method:" + methodName, currentScope);
            currentScope = methodScope;
            
            // Add parameters to method scope
            set<string> paramNames;
            for (auto& param : params) {
                if (paramNames.count(param.first)) {
                    errorReporter.report(node->lineno, "Duplicate parameter: '" + param.first + "'");
                } else {
                    paramNames.insert(param.first);
                    currentScope->addSymbol(param.first, param.second, SymbolKind::PARAMETER);
                }
            }
        }
        else if (node->type == "MainEntry") {
            string returnType = "int";
            for (auto child : node->children) {
                if (child->type == "Type") {
                    returnType = child->value;
                    break;
                }
            }
            currentScope->addMethodSymbol("main", returnType, {});
            currentDepth++;
            Scope* methodScope = new Scope(scopeCounter++, currentDepth, "Method:main", currentScope);
            currentScope = methodScope;
        }
        else if (node->type == "BlockContent") {
            if (parent && (parent->type == "Method" || parent->type == "MainEntry")) {
                // Don't create new scope for method body
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
        else if (node->type == "VarDeclaration") {
            addVariable(node);
        }
        else if (node->type == "Var" && parent && parent->type == "ClassContent") {
            // Class member variable (not inside VarDeclaration)
            addVariable(node);
        }
        
        for (auto child : node->children) {
            buildSymbolTable(child, node);
        }
        
        // Exit scope
        bool createdScope = (node->type == "Class" || node->type == "Method" || node->type == "MainEntry");
        if (node->type == "BlockContent" && !(parent && (parent->type == "Method" || parent->type == "MainEntry"))) {
            createdScope = true;
        }
        if (createdScope && currentScope->parent) {
            currentScope = currentScope->parent;
            currentDepth--;
            if (node->type == "Class") {
                currentClassName = "";
            }
        }
    }
    
    /* ========== PASS 2: Type Checking ========== */
    
    void typeCheck(Node* node, Node* parent = nullptr) {
        if (!node) return;
        
        // Enter scopes for context
        if (node->type == "Class") {
            currentClassName = node->value;
            Scope* classScope = globalTable.getClassScope(currentClassName);
            if (classScope) currentScope = classScope;
        }
        else if (node->type == "Method" || node->type == "MainEntry") {
            string methodName = (node->type == "MainEntry") ? "main" : node->value;
            
            // Find method scope
            for (auto child : currentScope->children) {
                if (child->name == "Method:" + methodName) {
                    currentScope = child;
                    break;
                }
            }
            
            // Get return type
            if (node->type == "MainEntry") {
                currentMethodReturnType = "int";
            } else {
                currentMethodReturnType = "void";
                for (auto childNode : node->children) {
                    if (childNode->type == "Type") {
                        currentMethodReturnType = childNode->value;
                        break;
                    }
                }
            }
            foundReturn = false;
        }
        else if (node->type == "BlockContent") {
            if (!(parent && (parent->type == "Method" || parent->type == "MainEntry"))) {
                // Enter block scope
                for (auto child : currentScope->children) {
                    string prefix = "BlockContent";
                    if (parent && parent->type == "ForStatement") prefix = "ForContent";
                    else if (parent && parent->type == "IfStatement") prefix = "IfContent";
                    if (child->name == prefix) {
                        currentScope = child;
                        break;
                    }
                }
            }
        }
        
        // Type check various node types
        if (node->type == "AssignStatement") {
            checkAssignment(node);
        }
        else if (node->type == "ReturnStatement") {
            checkReturn(node, parent);
            foundReturn = true;
        }
        else if (node->type == "PrintStatement") {
            // Print can accept int or boolean
            if (!node->children.empty()) {
                string exprType = getExpressionType(node->children.front());
            }
        }
        else if (node->type == "IfStatement") {
            checkIfStatement(node);
        }
        else if (node->type == "ForStatement") {
            checkForStatement(node);
        }
        else if (node->type == "VarDeclaration") {
            checkVariableDeclaration(node);
        }
        else if (node->type == "Var" && parent && parent->type == "ClassContent") {
            checkVariableDeclaration(node);
        }
        else if (node->type == "MethodCall") {
            checkMethodCall(node);
        }
        
        // Check for unreachable code after return
        if (parent && parent->type == "BlockContent") {
            bool afterReturn = false;
            for (auto child : parent->children) {
                if (afterReturn && child->type != "ReturnStatement") {
                    // This is a statement after return
                    if (child->type == "AssignStatement" || child->type == "PrintStatement" ||
                        child->type == "VarDeclaration" || child->type == "ExpressionStatement" ||
                        child->type == "IfStatement" || child->type == "ForStatement") {
                        errorReporter.report(child->lineno, "Unreachable statement after return");
                        break;
                    }
                }
                if (child->type == "ReturnStatement") {
                    afterReturn = true;
                }
            }
        }
        
        // Recurse
        for (auto child : node->children) {
            typeCheck(child, node);
        }
        
        // Exit scope
        if (node->type == "Class") {
            currentScope = currentScope->parent;
            currentClassName = "";
        }
        else if (node->type == "Method" || node->type == "MainEntry") {
            // Check for missing return statement
            if (currentMethodReturnType != "void" && !foundReturn) {
                errorReporter.report(node->lineno, "Missing return statement in non-void method");
            }
            currentScope = currentScope->parent;
            currentMethodReturnType = "";
        }
        else if (node->type == "BlockContent" && 
                 !(parent && (parent->type == "Method" || parent->type == "MainEntry"))) {
            currentScope = currentScope->parent;
        }
    }
    
private:
    void collectParameters(Node* paramList, vector<pair<string, string>>& params) {
        for (auto param : paramList->children) {
            if (param->type == "Param") {
                string name = "";
                string type = "";
                for (auto child : param->children) {
                    if (child->type == "Id") name = child->value;
                    if (child->type == "Type") type = child->value;
                }
                if (!name.empty() && !type.empty()) {
                    params.push_back({name, type});
                }
            }
        }
    }
    
    void addVariable(Node* node) {
        if (!node) return;
        
        string type = "unknown";
        string name = "unknown";
        
        // Handle VarDeclaration which may contain AssignStatement or just Var
        if (node->type == "VarDeclaration") {
            for (auto child : node->children) {
                if (child->type == "AssignStatement") {
                    for (auto subchild : child->children) {
                        if (subchild->type == "Var") {
                            for (auto varChild : subchild->children) {
                                if (varChild->type == "Type") type = varChild->value;
                                if (varChild->type == "Id") name = varChild->value;
                            }
                        }
                    }
                }
                else if (child->type == "Var") {
                    // VarDeclaration with just Var (no assignment)
                    for (auto varChild : child->children) {
                        if (varChild->type == "Type") type = varChild->value;
                        if (varChild->type == "Id") name = varChild->value;
                    }
                }
            }
        }
        else if (node->type == "Var") {
            for (auto child : node->children) {
                if (child->type == "Type") type = child->value;
                if (child->type == "Id") name = child->value;
            }
        }
        
        if (name != "unknown") {
            // Check for duplicate variable
            if (currentScope->existsInCurrentScope(name)) {
                errorReporter.report(node->lineno, "Duplicate variable declaration: '" + name + "'");
            } else {
                currentScope->addSymbol(name, type, SymbolKind::VARIABLE);
            }
        }
    }
    
    void checkVariableDeclaration(Node* node) {
        string type = "";
        
        if (node->type == "VarDeclaration") {
            for (auto child : node->children) {
                if (child->type == "AssignStatement") {
                    for (auto subchild : child->children) {
                        if (subchild->type == "Var") {
                            for (auto varChild : subchild->children) {
                                if (varChild->type == "Type") type = varChild->value;
                            }
                        }
                    }
                }
                else if (child->type == "Var") {
                    // VarDeclaration with just Var (no assignment)
                    for (auto varChild : child->children) {
                        if (varChild->type == "Type") type = varChild->value;
                    }
                }
            }
        // This might be redundant, should maybe be handled in checkAssignment
        } else if (node->type == "Var") {
            for (auto child : node->children) {
                if (child->type == "Type") type = child->value;
            }
        }
        
        // Check if type is valid
        if (!type.empty() && !globalTable.isValidType(type)) {
            errorReporter.report(node->lineno, "Unknown type: '" + type + "'");
        }
    }
    
    void checkAssignment(Node* node) {
        if (node->children.size() < 2) return;
        
        auto it = node->children.begin();
        Node* lhs = *it++;
        Node* rhs = *it;
        
        string lhsType = "";
        string lhsName = "";
        
        // Get LHS type
        if (lhs->type == "Var") {
            for (auto child : lhs->children) {
                if (child->type == "Type") lhsType = child->value;
                if (child->type == "Id") lhsName = child->value;
            }
        }
        else if (lhs->type == "Id") {
            lhsName = lhs->value;
            Symbol* sym = currentScope->lookup(lhsName);
            if (sym) {
                lhsType = sym->type;
            } else {
                // Implicit variable declaration (a := 5)
                // Infer type from RHS and add to symbol table
                string rhsType = getExpressionType(rhs);
                if (rhsType != "error" && rhsType != "unknown") {
                    currentScope->addSymbol(lhsName, rhsType, SymbolKind::VARIABLE);
                    lhsType = rhsType;
                }
                return;  // Skip type check since we just created it
            }
        }
        else if (lhs->type == "IndexExpression") {
            // Array element assignment a[i] := value
            checkIndexExpression(lhs);
            lhsType = getIndexExpressionElementType(lhs);  // Get actual element type
        }
        
        // Get RHS type
        string rhsType = getExpressionType(rhs);
        
        // Check type compatibility
        if (!lhsType.empty() && !rhsType.empty() && lhsType != rhsType && rhsType != "error") {
            errorReporter.report(node->lineno, "Type mismatch in assignment: cannot assign '" + rhsType + "' to '" + lhsType + "'");
        }
    }
    
    void checkReturn(Node* node, Node* parent) {
        if (currentMethodReturnType.empty()) return;
        
        if (node->children.empty()) {
            if (currentMethodReturnType != "void") {
                errorReporter.report(node->lineno, "Missing return value in non-void method");
            }
            return;
        }
        
        string returnType = getExpressionType(node->children.front());
        
        if (returnType != currentMethodReturnType && returnType != "error") {
            errorReporter.report(node->lineno, "Invalid return type: expected '" + currentMethodReturnType + "', got '" + returnType + "'");
        }
    }
    
    void checkIfStatement(Node* node) {
        // Find condition
        for (auto child : node->children) {
            if (child->type != "BlockContent" && child->type != "IfStatement") {
                string condType = getExpressionType(child);
                if (condType != "boolean" && condType != "error") {
                    errorReporter.report(child->lineno, "If condition must be boolean, got '" + condType + "'");
                }
                break;
            }
        }
    }
    
    void checkForStatement(Node* node) {
        for (auto child : node->children) {
            if (child->type == "end_condition") {
                for (auto cond : child->children) {
                    string condType = getExpressionType(cond);
                    if (condType != "boolean" && condType != "error") {
                        errorReporter.report(cond->lineno, "For loop condition must be boolean, got '" + condType + "'");
                    }
                }
            }
        }
    }
    
    void checkIndexExpression(Node* node) {
        if (node->children.size() < 2) return;
        
        auto it = node->children.begin();
        Node* array = *it++;
        Node* index = *it;
        
        // Check array type
        string arrayType = getExpressionType(array);
        if (!GlobalSymbolTable::isArrayType(arrayType) && arrayType != "error") {
            errorReporter.report(array->lineno, "Cannot index non-array type: '" + arrayType + "'");
        }
        
        // Check index type
        string indexType = getExpressionType(index);
        if (indexType != "int" && indexType != "error") {
            errorReporter.report(index->lineno, "Array index must be int, got '" + indexType + "'");
        }
    }
    
    // Get the element type of an index expression
    string getIndexExpressionElementType(Node* node) {
        if (node->children.empty()) return "error";
        
        Node* array = node->children.front();
        string arrayType = getExpressionType(array);
        
        if (GlobalSymbolTable::isArrayType(arrayType)) {
            return GlobalSymbolTable::getElementType(arrayType);
        }
        return "error";
    }
    
    void checkMethodCall(Node* node) {
        string methodName = "";
        string targetClass = "";
        vector<Node*> args;
        
        for (auto child : node->children) {
            if (child->type == "FieldAccess") {
                methodName = child->value;
                // Get target class from field access
                for (auto fc : child->children) {
                    if (fc->type == "Id") {
                        Symbol* sym = currentScope->lookup(fc->value);
                        if (sym) {
                            targetClass = sym->type;
                        }
                    }
                    else if (fc->type == "MethodCall") {
                        targetClass = getExpressionType(fc);
                    }
                }
            }
            else if (child->type == "Id") {
                // Constructor call or simple method call
                methodName = child->value;
                if (globalTable.getClassScope(methodName)) {
                    // Constructor call
                    targetClass = methodName;
                } else if (!currentClassName.empty()) {
                    targetClass = currentClassName;
                }
            }
            else if (child->type == "ArgList") {
                for (auto arg : child->children) {
                    args.push_back(arg);
                }
            }
        }
        
        // Validate method exists and check arguments
        if (!targetClass.empty() && !methodName.empty()) {
            // Check if it's a constructor call
            if (targetClass == methodName) {
                return;  // Constructor call is valid
            }
            
            Symbol* method = globalTable.lookupMethodInClass(targetClass, methodName);
            if (!method) {
                errorReporter.report(node->lineno, "Undefined method: '" + methodName + "' in class '" + targetClass + "'");
            } else if (method->methodInfo) {
                // Check argument count
                if (args.size() != method->methodInfo->parameters.size()) {
                    errorReporter.report(node->lineno, "Wrong number of arguments for method '" + methodName + 
                        "': expected " + to_string(method->methodInfo->parameters.size()) + 
                        ", got " + to_string(args.size()));
                } else {
                    // Check argument types
                    for (size_t i = 0; i < args.size(); i++) {
                        string argType = getExpressionType(args[i]);
                        string expectedType = method->methodInfo->parameters[i].second;
                        if (argType != expectedType && argType != "error") {
                            errorReporter.report(args[i]->lineno, "Invalid argument type for parameter '" + 
                                method->methodInfo->parameters[i].first + "': expected '" + 
                                expectedType + "', got '" + argType + "'");
                        }
                    }
                }
            }
        }
    }
    
    string getExpressionType(Node* node) {
        if (!node) return "error";
        
        if (node->type == "Int") {
            return "int";
        }
        else if (node->type == "Boolean" || node->type == "True" || node->type == "False") {
            return "boolean";
        }
        else if (node->type == "Id") {
            Symbol* sym = currentScope->lookup(node->value);
            if (sym) {
                return sym->type;
            } else {
                errorReporter.report(node->lineno, "Undefined variable: '" + node->value + "'");
                return "error";
            }
        }
        else if (node->type == "This") {
            if (!currentClassName.empty()) {
                return currentClassName;
            }
            errorReporter.report(node->lineno, "Invalid use of 'this' outside of class");
            return "error";
        }
        else if (node->type == "AddExpression" || node->type == "SubExpression" ||
                 node->type == "MulExpression" || node->type == "MultExpression" ||
                 node->type == "DivExpression" || node->type == "ExpExpression") {
            return checkArithmeticOp(node);
        }
        else if (node->type == "AndExpression" || node->type == "OrExpression") {
            return checkLogicalOp(node);
        }
        else if (node->type == "LtExpression" || node->type == "GtExpression" ||
                 node->type == "LeExpression" || node->type == "GeExpression" ||
                 node->type == "EqExpression" || node->type == "NeExpression") {
            return checkComparisonOp(node);
        }
        else if (node->type == "NotExpression") {
            return checkNotOp(node);
        }
        else if (node->type == "IndexExpression") {
            checkIndexExpression(node);
            return getIndexExpressionElementType(node);  // Return actual element type
        }
        else if (node->type == "LengthExpression") {
            return checkLengthExpression(node);
        }
        else if (node->type == "MethodCall") {
            return getMethodCallType(node);
        }
        else if (node->type == "ListExpression" || node->type == "NewArrayExpression") {
            // Infer array type from the baseType child node
            for (auto child : node->children) {
                if (child->type == "Type" || child->type == "BaseType") {
                    return child->value + "[]";
                }
            }
            return "int[]";  // Default fallback
        }
        else if (node->type == "NewExpression") {
            // new ClassName()
            for (auto child : node->children) {
                if (child->type == "Id") {
                    return child->value;
                }
            }
        }
        
        return "unknown";
    }
    
    string checkArithmeticOp(Node* node) {
        if (node->children.size() < 2) return "error";
        
        auto it = node->children.begin();
        string leftType = getExpressionType(*it++);
        string rightType = getExpressionType(*it);
        
        if (leftType != "int" && leftType != "error") {
            errorReporter.report(node->lineno, "Invalid operand type for arithmetic operation: expected 'int', got '" + leftType + "'");
            return "error";
        }
        if (rightType != "int" && rightType != "error") {
            errorReporter.report(node->lineno, "Invalid operand type for arithmetic operation: expected 'int', got '" + rightType + "'");
            return "error";
        }
        
        return "int";
    }
    
    string checkLogicalOp(Node* node) {
        if (node->children.size() < 2) return "error";
        
        auto it = node->children.begin();
        string leftType = getExpressionType(*it++);
        string rightType = getExpressionType(*it);
        
        if (leftType != "boolean" && leftType != "error") {
            errorReporter.report(node->lineno, "Invalid operand type for logical operation: expected 'boolean', got '" + leftType + "'");
            return "error";
        }
        if (rightType != "boolean" && rightType != "error") {
            errorReporter.report(node->lineno, "Invalid operand type for logical operation: expected 'boolean', got '" + rightType + "'");
            return "error";
        }
        
        return "boolean";
    }
    
    string checkComparisonOp(Node* node) {
        if (node->children.size() < 2) return "error";
        
        auto it = node->children.begin();
        string leftType = getExpressionType(*it++);
        string rightType = getExpressionType(*it);
        
        // For <, >, <=, >= both must be int
        if (node->type == "LtExpression" || node->type == "GtExpression" ||
            node->type == "LeExpression" || node->type == "GeExpression") {
            if (leftType != "int" && leftType != "error") {
                errorReporter.report(node->lineno, "Invalid operand type for comparison: expected 'int', got '" + leftType + "'");
                return "error";
            }
            if (rightType != "int" && rightType != "error") {
                errorReporter.report(node->lineno, "Invalid operand type for comparison: expected 'int', got '" + rightType + "'");
                return "error";
            }
        }
        // For ==, != both must be same type
        else if (leftType != rightType && leftType != "error" && rightType != "error") {
            errorReporter.report(node->lineno, "Cannot compare '" + leftType + "' with '" + rightType + "'");
            return "error";
        }
        
        return "boolean";
    }
    
    string checkNotOp(Node* node) {
        if (node->children.empty()) return "error";
        
        string operandType = getExpressionType(node->children.front());
        
        if (operandType != "boolean" && operandType != "error") {
            errorReporter.report(node->lineno, "Invalid operand type for '!': expected 'boolean', got '" + operandType + "'");
            return "error";
        }
        
        return "boolean";
    }
    
    string checkLengthExpression(Node* node) {
        if (node->children.empty()) return "error";
        
        string exprType = getExpressionType(node->children.front());
        
        // Accept any array type
        if (!GlobalSymbolTable::isArrayType(exprType) && exprType != "error") {
            errorReporter.report(node->lineno, "'.length' can only be used on arrays, got '" + exprType + "'");
            return "error";
        }
        
        return "int";
    }
    
    string getMethodCallType(Node* node) {
        string methodName = "";
        string targetClass = "";
        
        for (auto child : node->children) {
            if (child->type == "FieldAccess") {
                methodName = child->value;
                for (auto fc : child->children) {
                    if (fc->type == "Id") {
                        Symbol* sym = currentScope->lookup(fc->value);
                        if (sym) {
                            targetClass = sym->type;
                        } else {
                            // Could be a class name for constructor
                            if (globalTable.getClassScope(fc->value)) {
                                targetClass = fc->value;
                            }
                        }
                    }
                    else if (fc->type == "MethodCall") {
                        targetClass = getMethodCallType(fc);
                    }
                }
            }
            else if (child->type == "Id") {
                methodName = child->value;
                // Constructor call
                if (globalTable.getClassScope(methodName)) {
                    return methodName;
                }
                // Method in current class
                if (!currentClassName.empty()) {
                    targetClass = currentClassName;
                }
            }
        }
        
        if (!targetClass.empty() && !methodName.empty()) {
            if (targetClass == methodName) {
                return targetClass;  // Constructor
            }
            
            Symbol* method = globalTable.lookupMethodInClass(targetClass, methodName);
            if (method) {
                return method->type;
            }
        }
        
        return "error";
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
        
        generateScopeDotNode(dotOut, globalScope);
        generateScopeDotEdges(dotOut, globalScope);
        
        dotOut << "}" << endl;
        dotOut.close();
    }
    
private:
    void generateScopeDotNode(ofstream& out, Scope* scope) {
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
    
    void generateScopeDotEdges(ofstream& out, Scope* scope) {
        for (auto child : scope->children) {
            out << "    scope_" << scope->id << " -> scope_" << child->id << ";" << endl;
            generateScopeDotEdges(out, child);
        }
    }
};

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Tree Parser - Builds AST from tree.dot */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

Node* parseTreeDot(const string& filename) {
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
            size_t labelEnd = line.rfind("\"");
            
            if (nodeIdStart != string::npos && labelStart < labelEnd) {
                string nodeId = line.substr(nodeIdStart, nodeIdEnd - nodeIdStart);
                string label = line.substr(labelStart, labelEnd - labelStart);
                
                int lineNo = 0;
                if (nodeId.length() > 1) {
                    lineNo = stoi(nodeId.substr(1));
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

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Main */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

int main(int argc, char* argv[]) {
    string treeFile = "tree.dot";
    
    Node* root = parseTreeDot(treeFile);
    if (!root) {
        cerr << "Error: Could not parse AST" << endl;
        return 1;
    }
    
    ErrorReporter errorReporter;
    SemanticAnalyzer analyzer(errorReporter);
    
    // Pass 1: Build symbol table
    cout << "=== Pass 1: Building Symbol Table ===" << endl;
    analyzer.buildSymbolTable(root);
    
    // Generate symbol table visualization
    analyzer.generateSymbolTableDot();
    
    // Pass 2: Type checking
    cout << "=== Pass 2: Type Checking ===" << endl;
    analyzer.typeCheck(root);
    
    // Summary
    errorReporter.printSummary();
    
    return errorReporter.hasErrors() ? 1 : 0;
}
