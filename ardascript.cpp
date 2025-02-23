// Copyright (C) 2025 Arda Koyuncu
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU GPLv3.

#include <iostream>
#include <string>
#include <cctype>
#include <sstream>
#include <regex>
#include <vector>
#include <stack>
#include <map>
#include <variant>
using namespace std;

bool Error = false;
string ErrorMessage = "";
map<string, variant<int, double, string, bool>> variables;
map<string, string> variableTypes;

// Prototipler
string checkBrackets(string code, char bracket, char bracket2);
bool isHave(string metin, string kelime);
vector<string> tokenize(const string &expr);
vector<string> infixToPostfix(const vector<string> &tokens);
double evaluatePostfix(const vector<string> &postfix);
string detectType(string code);
void executeLine(string line);
vector<string> split(const string &s, char delimiter);
void executeBlock(vector<string> &lines, size_t &currentLine);
void parseVariableDeclaration(string line);
void executeArdaScript(string code);
void handleIfElse(vector<string>& lines, size_t &currentLine);

void handleWhile(vector<string>& lines, size_t &currentLine) {
    string condition = checkBrackets(lines[currentLine], '(', ')');
    if (Error) return;
    
    size_t loopStartLine = currentLine; // Döngü başlangıç pozisyonu
    currentLine++; // while satırını atla
    
    while (true) {
        // Koşulu kontrol et
        vector<string> tokens = tokenize(condition);
        vector<string> postfix = infixToPostfix(tokens);
        double result = evaluatePostfix(postfix);
        
        if (Error || result == 0) break; // Koşul sağlanmıyorsa çık
        
        // Döngü gövdesini çalıştır
        executeBlock(lines, currentLine);
        
        // Pozisyonu resetle (aynı döngüyü tekrar işlemek için)
        currentLine = loopStartLine + 1;
    }
}

// Tokenize fonksiyonu
vector<string> tokenize(const string &expr) {
    vector<string> tokens;
    string current;
    bool readingNumber = false;
    bool readingIdentifier = false;

    for (size_t i = 0; i < expr.size(); ++i) {
        char c = expr[i];
        if (isspace(c)) continue;

        if (string("><=!").find(c) != string::npos) {
            if (readingNumber || readingIdentifier) {
                tokens.push_back(current);
                current.clear();
                readingNumber = false;
                readingIdentifier = false;
            }
            if (i + 1 < expr.size()) {
                char nextC = expr[i+1];
                string op(1, c);
                op += nextC;
                if (op == "==" || op == "!=" || op == ">=" || op == "<=") {
                    tokens.push_back(op);
                    i++;
                    continue;
                }
            }
            tokens.push_back(string(1, c));
            continue;
        }

        if (isalpha(c)) {
            if (readingNumber) {
                tokens.push_back(current);
                current.clear();
                readingNumber = false;
            }
            current += c;
            readingIdentifier = true;
        }
        else if (isdigit(c) || c == '.' || (c == '-' && (i == 0 || expr[i-1] == '(' || 
            string("+-*/").find(expr[i-1]) != string::npos))) {
            if (readingIdentifier) {
                tokens.push_back(current);
                current.clear();
                readingIdentifier = false;
            }
            current += c;
            readingNumber = true;
        } else {
            if (readingNumber || readingIdentifier) {
                tokens.push_back(current);
                current.clear();
                readingNumber = false;
                readingIdentifier = false;
            }
            if (string("+-*/()").find(c) != string::npos) {
                tokens.push_back(string(1, c));
            } else {
                Error = true;
                ErrorMessage = "Invalid character: " + string(1, c);
                return {};
            }
        }
    }

    if (readingNumber || readingIdentifier) tokens.push_back(current);
    return tokens;
}

// Infix to Postfix dönüşümü
vector<string> infixToPostfix(const vector<string>& tokens) {
    vector<string> output;
    stack<string> opStack;
    map<string, int> precedence{
        {"+",1}, {"-",1}, {"*",2}, {"/",2},
        {">",3}, {"<",3}, {">=",3}, {"<=",3}, {"==",3}, {"!=",3}
    };

    for (const auto& token : tokens) {
        if (isdigit(token[0]) || (token.size() > 1 && token[0] == '-') || isalpha(token[0])) {
            output.push_back(token);
        } else if (token == "(") {
            opStack.push(token);
        } else if (token == ")") {
            while (!opStack.empty() && opStack.top() != "(") {
                output.push_back(opStack.top());
                opStack.pop();
            }
            if (opStack.empty()) {
                Error = true;
                ErrorMessage = "Bracket error";
                return {};
            }
            opStack.pop();
        } else {
            while (!opStack.empty() && opStack.top() != "(" && 
                   precedence[opStack.top()] >= precedence[token]) {
                output.push_back(opStack.top());
                opStack.pop();
            }
            opStack.push(token);
        }
    }

    while (!opStack.empty()) {
        if (opStack.top() == "(") {
            Error = true;
            ErrorMessage = "Bracket error";
            return {};
        }
        output.push_back(opStack.top());
        opStack.pop();
    }

    return output;
}
// Postfix değerlendirme
double evaluatePostfix(const vector<string>& postfix) {
    stack<double> st;
    for (const auto& token : postfix) {
        if (isdigit(token[0]) || (token.size() > 1 && token[0] == '-')) {
            st.push(stod(token));
        } 
        else if (isalpha(token[0])) {
            if (variables.find(token) != variables.end()) {
                if (holds_alternative<int>(variables[token])) {
                    st.push(get<int>(variables[token]));
                } else if (holds_alternative<double>(variables[token])) {
                    st.push(get<double>(variables[token]));
                }
            } else {
                Error = true;
                ErrorMessage = "Undefined variable: " + token;
                return 0;
            }
        } else {
            double b = st.top(); st.pop();
            double a = st.top(); st.pop();
            if (token == "+") st.push(a + b);
            else if (token == "-") st.push(a - b);
            else if (token == "*") st.push(a * b);
            else if (token == "/") {
                if (b == 0) {
                    Error = true;
                    ErrorMessage = "Division by zero";
                    return 0;
                }
                st.push(a / b);
            }
            else if (token == ">") st.push(a > b ? 1 : 0);
            else if (token == "<") st.push(a < b ? 1 : 0);
            else if (token == ">=") st.push(a >= b ? 1 : 0);
            else if (token == "<=") st.push(a <= b ? 1 : 0);
            else if (token == "==") st.push(a == b ? 1 : 0);
            else if (token == "!=") st.push(a != b ? 1 : 0);
        }
    }
    return st.top();
}

// Parantez kontrolü
string checkBrackets(string code, char bracket, char bracket2) {
    int count = 0;
    size_t start = string::npos;
    string content;

    for (size_t i = 0; i < code.size(); ++i) {
        if (code[i] == bracket) {
            if (count++ == 0) start = i + 1;
        } else if (code[i] == bracket2 && count > 0) {
            if (--count == 0) {
                content = code.substr(start, i - start);
                return content;
            }
        }
    }

    Error = true;
    ErrorMessage = "Bracket error";
    return "";
}

// String split fonksiyonu
vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        if (!token.empty()) tokens.push_back(token);
    }
    return tokens;
}

// Değişken tanımlama
void parseVariableDeclaration(string line) {
    smatch match;
    if (regex_match(line, match, regex(R"((int|double|string|bool)\s+(\w+)\s*=\s*(.+))"))) {
        string type = match[1];
        string name = match[2];
        string value = match[3];

        if (variableTypes.find(name) != variableTypes.end()) {
            Error = true;
            ErrorMessage = "Variable '" + name + "' already declared";
            return;
        }

        variableTypes[name] = type;

        try {
            if (type == "int") {
                variables[name] = stoi(value);
            } else if (type == "double") {
                variables[name] = stod(value);
            } else if (type == "bool") {
                // "true" veya "1" ise true, diğer durumlarda false
                variables[name] = (value == "true" || value == "1");
            } else { // string
                // Tırnak işaretlerini kaldır
                variables[name] = value.substr(1, value.size()-2);
            }
        } catch (...) {
            Error = true;
            ErrorMessage = "Invalid value for variable '" + name + "'";
        }
    }
}

// If-else yönetimi
void handleIfElse(vector<string>& lines, size_t &currentLine) {
    string condition = checkBrackets(lines[currentLine], '(', ')');
    if (Error) return;

    vector<string> tokens = tokenize(condition);
    vector<string> postfix = infixToPostfix(tokens);
    double result = evaluatePostfix(postfix);
    
    currentLine++; // if satırını atla
    
    if (result != 0) {
        executeBlock(lines, currentLine); // if bloğunu çalıştır
    } else {
        // Else bloğunu ara
        while (currentLine < lines.size()) {
            if (lines[currentLine].find("else") != string::npos) {
                currentLine++; // else satırını atla
                executeBlock(lines, currentLine);
                return;
            }
            currentLine++;
        }
    }
}

// Kod bloğu yürütme
void executeBlock(vector<string>& lines, size_t &currentLine) {
    while (currentLine < lines.size()) {
        string line = lines[currentLine];
        line = regex_replace(line, regex("^\\s+|\\s+$"), "");

        if (line.empty()) {
            currentLine++;
            continue;
        }

        if (line.find("}") != string::npos) {
            currentLine++;
            return;
        }

        if (isHave(line, "if")) {
            handleIfElse(lines, currentLine);
        } 
        else if (isHave(line, "while")) { // While kontrolü eklendi
            handleWhile(lines, currentLine);
        }
        else {
            executeLine(line);
            currentLine++;
        }
    }
}

// Satır yürütme
void executeLine(string line) {
    // Değişken tanımlama kısmı (int, double, string, bool)
    if (regex_search(line, regex(R"(^(int|double|string|bool)\s)"))) {
        parseVariableDeclaration(line);
        return;
    }

    // Atama işlemleri (BU BÖLÜMÜ GÜNCELLEYİN)
    if (line.find('=') != string::npos) {
        vector<string> parts = split(line, '=');
        if (parts.size() != 2) {
            Error = true;
            ErrorMessage = "Invalid assignment";
            return;
        }

        string varName = regex_replace(parts[0], regex("\\s+"), "");
        string valueStr = regex_replace(parts[1], regex("^\\s+|\\s+$"), "");

        // YENİ EKLENEN KOD (Matematiksel ifade çözümleme)
        vector<string> tokens = tokenize(valueStr);
        if (!Error) {
            vector<string> postfix = infixToPostfix(tokens);
            if (!Error) {
                double result = evaluatePostfix(postfix);
                if (!Error) {
                    variables[varName] = result;
                    variableTypes[varName] = "double";
                    return; // Başarılıysa buradan çık
                }
            }
        }

        // Eski tip kontrolü (string/bool vs.)
        string valueType = detectType(valueStr);
        
        try {
            if (valueType == "int") {
                variables[varName] = stoi(valueStr);
            } else if (valueType == "double") {
                variables[varName] = stod(valueStr);
            } else if (valueType == "bool") {
                variables[varName] = (valueStr == "true" || valueStr == "1");
            } else {
                variables[varName] = valueStr;
            }
            variableTypes[varName] = valueType;
        } catch (...) {
            Error = true;
            ErrorMessage = "Invalid value assignment";
        }
        return;
    }

    // Print ifadesi
    if (isHave(line, "print")) {
        string content = checkBrackets(line, '(', ')');
        if (Error) return;
    
        vector<string> printArgs;
        size_t start = 0;
        bool inString = false;
        
        // Virgülleri dikkate alarak parçala
        for (size_t i = 0; i < content.size(); ++i) {
            if (content[i] == '"') inString = !inString;
            if (content[i] == ',' && !inString) {
                printArgs.push_back(content.substr(start, i - start));
                start = i + 1;
            }
        }
        printArgs.push_back(content.substr(start));
        
        // Her bir argümanı işle
        for (string arg : printArgs) {
            arg = regex_replace(arg, regex("^\\s+|\\s+$"), "");
            
            // Değişken kontrolü
            if (variables.find(arg) != variables.end()) {
                visit([](auto&& val) { cout << val << " "; }, variables[arg]);
            }
            // String literal
            else if (arg.size() >= 2 && arg[0] == '"' && arg.back() == '"') {
                cout << arg.substr(1, arg.size()-2) << " ";
            }
            // Matematiksel ifade
            else {
                vector<string> tokens = tokenize(arg);
                if (!Error) {
                    vector<string> postfix = infixToPostfix(tokens);
                    if (!Error) {
                        double result = evaluatePostfix(postfix);
                        if (!Error) cout << result << " ";
                    }
                }
            }
        }
        cout << endl;
    }
}

// Kelime kontrolü
bool isHave(string metin, string kelime) {
    regex pattern("\\b" + kelime + "\\b");
    return regex_search(metin, pattern);
}

// Tip belirleme
string detectType(string code) {
    if (!code.empty()) {
        if (code.front() == '"' && code.back() == '"') {
            return "string";
        } else if (code.front() == '\'' && code.back() == '\'') {
            return "char";
        }
    }

    if (code == "true" || code == "false") return "bool";

    try {
        stoi(code);
        return "int";
    } catch (...) {}

    try {
        stod(code);
        return "double";
    } catch (...) {}

    return "unknown";
}

// Ana yürütme fonksiyonu
void executeArdaScript(string code) {
    vector<string> lines;
    istringstream ss(code);
    string line;
    while (getline(ss, line)) {
        lines.push_back(line);
    }

    size_t currentLine = 0;
    executeBlock(lines, currentLine);
    
    if(Error){
        cout << currentLine << ". LINE ERROR: " << ErrorMessage << endl;
    }
}

/*
ArdaScript Kod Örnekleri:

!! Temel Değişken Tanımlamaları
int sayi = 10
double pi = 3.14
string mesaj = "Merhaba Dünya!"
bool dogruMu = true

! Dinamik Tip Atama
a = 25
b = "ArdaScript"
c = 5 + 3.2

! Matematiksel İşlemler
sonuc = (5 + 3) * 2 - 4 / 2
print("Sonuç: " + sonuc)

! Koşullu İfadeler
if (sonuc > 10) {
    print("Büyük sayı!")
} else {
    print("Küçük sayı")
}

! While Döngüsü
i = 0
while (i < 100) {
    print(i)
    i = i + 1
}

DAHA SONRA GELECEKLER
! Fonksiyon Tanımlama
function topla(a, b) {
    return a + b
}
print(topla(3, 7))

! Liste Yapısı
liste = [1, 2, 3, 4]
print(liste[2]) // 3 yazdırır

! String İşlemleri
ad = "Arda"
print(ad + "Script") // ArdaScript yazdırır

! Tip Kontrollü Değişken
int yas = 25
yas = "yirmibeş" // Hata verecek
*/

// Main fonksiyonu
int main() {
    string inputCode;
    string line;
    
    cout << "ArdaScript Interpreter (Type 'exit' to quit)" << endl;
    while(true) {
        cout << "> ";
        getline(cin, line);
        if(line == "exit") break;
        inputCode += line + "\n";
    }
    
    executeArdaScript(inputCode);
    
    cout << "\nProgram Finished.";
    cin.ignore(); // Buffer'ı temizle
    cin.get();     // Tek bir tuş bekler
    
    return 0;
}