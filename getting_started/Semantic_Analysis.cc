/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Semantic Analysis for C+- Compiler */
/* Part 2: Type Checking and Semantic Error Detection */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

#include <iostream>
#include <set>
#include "Node.h"
#include "SymbolTable.h"

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Semantic Error Reporter */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

class ErrorReporter {
private:
    int errorCount_ = 0;
    
public:
    void report(int line, const string& message) {
        errorCount_++;
        cerr << "Semantic Error (line " << line << "): " << message << endl;
    }
    
    bool hasErrors() const { return errorCount_ > 0; }
    
    int errorCount() const { return errorCount_; }
    
    void printSummary() {
        if (errorCount_ == 0) {
            cout << "No semantic errors found." << endl;
        } else {
            cout << "\n=== Semantic Analysis Summary ===" << endl;
            cout << "Total errors: " << errorCount_ << endl;
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
            checkReturn(node);
            foundReturn = true;
        }
        else if (node->type == "PrintStatement") {
            checkPrint(node);
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
    
    void checkReturn(Node* node) {
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
    
    void checkPrint(Node* node) {
        if (node->children.empty()) return;
        
        string exprType = getExpressionType(node->children.front());
        
        if (exprType != "int" && exprType != "boolean" && exprType != "error") {
            errorReporter.report(node->lineno, "Cannot print type '" + exprType + "': only int and boolean are printable");
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
    
    // Helper to extract method call info (methodName, targetClass, args)
    // Returns true if it's a constructor call
    bool extractMethodCallInfo(Node* node, string& methodName, string& targetClass, vector<Node*>& args) {
        for (auto child : node->children) {
            if (child->type == "FieldAccess") {
                methodName = child->value;
                for (auto fc : child->children) {
                    if (fc->type == "Id") {
                        Symbol* sym = currentScope->lookup(fc->value);
                        if (sym) {
                            targetClass = sym->type;
                        } else if (globalTable.getClassScope(fc->value)) {
                            targetClass = fc->value;
                        }
                    }
                    else if (fc->type == "MethodCall") {
                        targetClass = getMethodCallType(fc);
                    }
                }
            }
            else if (child->type == "Id") {
                methodName = child->value;
                if (globalTable.getClassScope(methodName)) {
                    targetClass = methodName;
                    return true;  // Constructor call
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
        return (targetClass == methodName);  // Constructor if class == method name
    }
    
    void checkMethodCall(Node* node) {
        string methodName, targetClass;
        vector<Node*> args;
        
        if (extractMethodCallInfo(node, methodName, targetClass, args)) {
            return;  // Constructor call is valid
        }
        
        if (targetClass.empty() || methodName.empty()) return;
        
        Symbol* method = globalTable.lookupMethodInClass(targetClass, methodName);
        if (!method) {
            errorReporter.report(node->lineno, "Undefined method: '" + methodName + "' in class '" + targetClass + "'");
        } else if (method->methodInfo) {
            if (args.size() != method->methodInfo->parameters.size()) {
                errorReporter.report(node->lineno, "Wrong number of arguments for method '" + methodName + 
                    "': expected " + to_string(method->methodInfo->parameters.size()) + 
                    ", got " + to_string(args.size()));
            } else {
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
    
    // Helper for binary operations
    string checkBinaryOp(Node* node, const string& expectedType, const string& opName) {
        if (node->children.size() < 2) return "error";
        
        auto it = node->children.begin();
        string leftType = getExpressionType(*it++);
        string rightType = getExpressionType(*it);
        
        if (leftType != expectedType && leftType != "error") {
            errorReporter.report(node->lineno, "Invalid operand type for " + opName + ": expected '" + expectedType + "', got '" + leftType + "'");
            return "error";
        }
        if (rightType != expectedType && rightType != "error") {
            errorReporter.report(node->lineno, "Invalid operand type for " + opName + ": expected '" + expectedType + "', got '" + rightType + "'");
            return "error";
        }
        
        return expectedType;
    }
    
    string checkArithmeticOp(Node* node) {
        return checkBinaryOp(node, "int", "arithmetic operation");
    }
    
    string checkLogicalOp(Node* node) {
        return checkBinaryOp(node, "boolean", "logical operation");
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
        string methodName, targetClass;
        vector<Node*> args;  // unused but needed for helper
        
        if (extractMethodCallInfo(node, methodName, targetClass, args)) {
            return targetClass;  // Constructor returns the class type
        }
        
        if (!targetClass.empty() && !methodName.empty()) {
            Symbol* method = globalTable.lookupMethodInClass(targetClass, methodName);
            if (method) {
                return method->type;
            }
        }
        
        return "error";
    }
    
public:
    void generateSymbolTableDot() {
        generateSymbolTableDotFile(globalScope);
    }
};

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
