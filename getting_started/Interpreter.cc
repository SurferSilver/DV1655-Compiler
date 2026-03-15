#include <iostream>
#include <fstream>
#include <string>
#include <stack>
#include <vector>
#include <map>
#include <sstream>
#include <cmath>



using namespace std;

namespace {

bool isNumber(const string& token) {
    if (token.empty()) return false;
    char* end = nullptr;
    strtod(token.c_str(), &end);
    return end && *end == '\0';
}

double readValue(const string& token, const map<string, double>& vars) {
    if (isNumber(token)) return stod(token);
    auto it = vars.find(token);
    return (it == vars.end()) ? 0.0 : it->second;
}

double popOrFail(stack<double>& st, const string& opcode) {
    if (st.empty()) {
        throw runtime_error("Stack underflow in opcode '" + opcode + "'");
    }
    double v = st.top();
    st.pop();
    return v;
}

vector<string> tokenize(const string& line) {
    istringstream iss(line);
    vector<string> tokens;
    string token;
    while (iss >> token) tokens.push_back(token);
    return tokens;
}

struct Frame {
    map<string, double> locals;
    vector<double> args;
    int returnPc;

    Frame(vector<double> callArgs, int retPc)
        : args(std::move(callArgs)), returnPc(retPc) {}
};

double getVarValue(const string& name,
                   const map<string, double>& globals,
                   const vector<Frame>& callStack) {
    if (!callStack.empty()) {
        const auto& locals = callStack.back().locals;
        auto lit = locals.find(name);
        if (lit != locals.end()) return lit->second;
    }

    auto git = globals.find(name);
    return (git == globals.end()) ? 0.0 : git->second;
}

void setVarValue(const string& name,
                 double value,
                 map<string, double>& globals,
                 vector<Frame>& callStack) {
    if (!callStack.empty()) {
        auto& locals = callStack.back().locals;
        if (locals.find(name) != locals.end()) {
            locals[name] = value;
            return;
        }
    }

    globals[name] = value;
}

double readTokenValue(const string& token,
                      const map<string, double>& globals,
                      const vector<Frame>& callStack) {
    if (isNumber(token)) return stod(token);
    return getVarValue(token, globals, callStack);
}

}

/*--------------------------------------------------------------------------------------------------------------------------------*/
// Main
/*--------------------------------------------------------------------------------------------------------------------------------*/


int main(int argc, char** argv) {
    //error handling
    if (argc < 2) {
        cerr << "Usage: interpreter <bytecode_file>" << endl;
        return 1;
    }

    ifstream file(argv[1]);
    if (!file.is_open()) {
        cerr << "Error: cannot open file '" << argv[1] << "'" << endl;
        return 1;
    }

    //getall instructions
    vector<string> instructions;
    string line;
    while (getline(file, line)) {
        if (!line.empty())
            instructions.push_back(line);
    }
    file.close();

    map<string, int> labels;
    for (int i = 0; i < static_cast<int>(instructions.size()); ++i) {
        vector<string> tokens = tokenize(instructions[i]);
        if (tokens.empty()) continue;

        if (!tokens[0].empty() && tokens[0].back() == ':') {
            string label = tokens[0].substr(0, tokens[0].size() - 1);
            labels[label] = i;
        } else if (tokens[0] == "LABEL" && tokens.size() >= 2) {
            labels[tokens[1]] = i;
        }
    }

    //initialize runtime components
    stack<double> values;
    map<string, double> globals;
    vector<double> pendingArgs;
    vector<Frame> callStack;

    bool halted = false;
    for (int pc = 0; pc < static_cast<int>(instructions.size()) && !halted; ++pc) {
        vector<string> tokens = tokenize(instructions[pc]);
        if (tokens.empty()) continue;

        if (!tokens[0].empty() && tokens[0].back() == ':') {
            continue;
        }

        const string& op = tokens[0];

        //Execute instruction based on opcode
        try {
            if (op == "HALT") {
                halted = true;

            } else if (op == "LABEL") {
                continue;

            } else if (op == "DECLARE") {
                if (tokens.size() < 2) throw runtime_error("Missing variable name for DECLARE");
                if (!callStack.empty()) {
                    callStack.back().locals[tokens[1]] = 0.0;
                } else {
                    globals[tokens[1]] = 0.0;
                }

            } else if (op == "DECLARE_VOLATILE") {
                if (tokens.size() < 2) throw runtime_error("Missing variable name for DECLARE_VOLATILE");
                if (!callStack.empty()) {
                    callStack.back().locals[tokens[1]] = 0.0;
                } else {
                    globals[tokens[1]] = 0.0;
                }
                
            } else if (op == "PUSH") {
                if (tokens.size() < 2) throw runtime_error("Missing operand for PUSH");
                values.push(readTokenValue(tokens[1], globals, callStack));
            
            } else if (op == "LOAD") {
                if (tokens.size() < 2) throw runtime_error("Missing operand for LOAD");
                values.push(readTokenValue(tokens[1], globals, callStack));
            
            } else if (op == "STORE") {
                if (tokens.size() < 2) throw runtime_error("Missing operand for STORE");
                setVarValue(tokens[1], popOrFail(values, op), globals, callStack);

            } else if (op == "STORE_VOLATILE") {

                if (tokens.size() < 2) throw runtime_error("Missing operand for STORE_VOLATILE");
                setVarValue(tokens[1], popOrFail(values, op), globals, callStack);

            } else if (op == "GET_PARAM") {
                if (tokens.size() < 3) throw runtime_error("Missing operands for GET_PARAM");
                if (callStack.empty()) throw runtime_error("GET_PARAM used outside function call");
                int idx = stoi(tokens[2]);
                if (idx < 0 || idx >= static_cast<int>(callStack.back().args.size())) {
                    throw runtime_error("GET_PARAM index out of range");
                }
                callStack.back().locals[tokens[1]] = callStack.back().args[idx];
            
            } else if (op == "ADD" || op == "SUB" || op == "MUL" || op == "DIV" || op == "EXP" ||
                       op == "EQ" || op == "NEQ" || op == "LT" || op == "GT" || op == "LEQ" ||
                       op == "GEQ" || op == "AND" || op == "OR") {
                double rhs = popOrFail(values, op);
                double lhs = popOrFail(values, op);

                if (op == "ADD") values.push(lhs + rhs);
                else if (op == "SUB") values.push(lhs - rhs);
                else if (op == "MUL") values.push(lhs * rhs);
                else if (op == "DIV") {
                    if (rhs == 0.0) throw runtime_error("Division by zero");
                    values.push(lhs / rhs);
                } else if (op == "EXP") values.push(pow(lhs, rhs));
                else if (op == "EQ") values.push(lhs == rhs ? 1.0 : 0.0);
                else if (op == "NEQ") values.push(lhs != rhs ? 1.0 : 0.0);
                else if (op == "LT") values.push(lhs < rhs ? 1.0 : 0.0);
                else if (op == "GT") values.push(lhs > rhs ? 1.0 : 0.0);
                else if (op == "LEQ") values.push(lhs <= rhs ? 1.0 : 0.0);
                else if (op == "GEQ") values.push(lhs >= rhs ? 1.0 : 0.0);
                else if (op == "AND") values.push((lhs != 0.0 && rhs != 0.0) ? 1.0 : 0.0);
                else if (op == "OR") values.push((lhs != 0.0 || rhs != 0.0) ? 1.0 : 0.0);
            
            } else if (op == "NOT") {
                double v = popOrFail(values, op);
                values.push(v == 0.0 ? 1.0 : 0.0);
            
            } else if (op == "PRINT") {
                cout << popOrFail(values, op) << endl;
            
            } else if (op == "READ") {
                if (tokens.size() < 2) throw runtime_error("Missing destination for READ");
                double input;
                if (!(cin >> input)) throw runtime_error("Failed to read numeric input");
                setVarValue(tokens[1], input, globals, callStack);
            
            } else if (op == "POP") {
                (void)popOrFail(values, op);
            
            } else if (op == "DUP") {
                double v = popOrFail(values, op);
                values.push(v);
                values.push(v);
            
            } else if (op == "PARAM") {
                pendingArgs.push_back(popOrFail(values, op));
            
            } else if (op == "CALL") {
                if (tokens.size() < 3) throw runtime_error("Missing operands for CALL");
                int argc = stoi(tokens[2]);
                if (argc < 0 || argc > static_cast<int>(pendingArgs.size())) {
                    throw runtime_error("Invalid argument count for CALL");
                }
                auto it = labels.find(tokens[1]);
                if (it == labels.end()) {
                    //not found in labels, noop
                    values.push(0.0);
                } else {
                    vector<double> args(pendingArgs.end() - argc, pendingArgs.end());
                    pendingArgs.resize(pendingArgs.size() - argc);
                    callStack.emplace_back(args, pc);
                    pc = it->second;
                }
            //Return RET if no return value, otherwise RETVAL 
            } else if (op == "RET" || op == "RETVAL") {
                double retValue = 0.0;
                if (op == "RETVAL") {
                    retValue = popOrFail(values, op);
                }

                // If not in a function call, halt execution (main return)
                if (callStack.empty()) {
                    halted = true;
                } else {
                    int returnPc = callStack.back().returnPc;
                    callStack.pop_back();
                    values.push(retValue);
                    pc = returnPc;
                }
            
            } else if (op == "JUMP") {
                if (tokens.size() < 2) throw runtime_error("Missing label for JUMP");
                auto it = labels.find(tokens[1]);
                if (it == labels.end()) throw runtime_error("Unknown label '" + tokens[1] + "'");
                pc = it->second;
            
            } else if (op == "JUMP_IF") {
                if (tokens.size() < 2) throw runtime_error("Missing label for JUMP_IF");
                double cond = popOrFail(values, op);
                if (cond != 0.0) {
                    auto it = labels.find(tokens[1]);
                    if (it == labels.end()) throw runtime_error("Unknown label '" + tokens[1] + "'");
                    pc = it->second;
                }
            
            } else if (op == "JUMP_IFNOT") {
                if (tokens.size() < 2) throw runtime_error("Missing label for JUMP_IFNOT");
                double cond = popOrFail(values, op);
                if (cond == 0.0) {
                    auto it = labels.find(tokens[1]);
                    if (it == labels.end()) throw runtime_error("Unknown label '" + tokens[1] + "'");
                    pc = it->second;
                }
            //Thin Ice, do not walk!
            } else if (op == "NEW_ARRAY") {
                if (values.empty()) throw runtime_error("Stack underflow for NEW_ARRAY");
                int size = static_cast<int>(popOrFail(values, op));
                static std::vector<std::vector<double>> arrayTable;
                arrayTable.emplace_back(size, 0.0);
                double arrRef = static_cast<double>(arrayTable.size() - 1);
                values.push(arrRef);
            } else if (op == "ARRAY_LOAD") {
                if (values.size() < 2) throw runtime_error("Stack underflow for ARRAY_LOAD");
                int idx = static_cast<int>(popOrFail(values, op));
                int arrRef = static_cast<int>(popOrFail(values, op));
                static std::vector<std::vector<double>> arrayTable;
                if (arrRef < 0 || arrRef >= static_cast<int>(arrayTable.size())) throw runtime_error("Invalid array reference in ARRAY_LOAD");
                auto& arr = arrayTable[arrRef];
                if (idx < 0 || idx >= static_cast<int>(arr.size())) throw runtime_error("Array index out of bounds in ARRAY_LOAD");
                values.push(arr[idx]);
            } else if (op == "ARRAY_STORE") {
                if (values.size() < 3) throw runtime_error("Stack underflow for ARRAY_STORE");
                double val = popOrFail(values, op);
                int idx = static_cast<int>(popOrFail(values, op));
                int arrRef = static_cast<int>(popOrFail(values, op));
                static std::vector<std::vector<double>> arrayTable;
                if (arrRef < 0 || arrRef >= static_cast<int>(arrayTable.size())) throw runtime_error("Invalid array reference in ARRAY_STORE");
                auto& arr = arrayTable[arrRef];
                if (idx < 0 || idx >= static_cast<int>(arr.size())) throw runtime_error("Array index out of bounds in ARRAY_STORE");
                arr[idx] = val;
            } else {
                throw runtime_error("Unknown opcode '" + op + "'");
            }
        } catch (const exception& ex) {
            cerr << "Runtime error at line " << (pc + 1) << ": " << ex.what() << endl;
            return 1;
        }
    }

    if (!halted) {
        cerr << "Warning: program terminated without HALT" << endl;
    }

    return 0;
}
