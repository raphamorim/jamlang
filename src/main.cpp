/*
 * Copyright (c) 2025 Raphael Amorim
 *
 * This file is part of jam, which is MIT licensed.
 * See http://opensource.org/licenses/MIT
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <string>
#include <stdexcept>
#include <map>
#include <optional>
#include <cstdint>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/CodeGen/TargetPassConfig.h"

// Token types
enum TokenType {
    TOK_EOF = 0,
    TOK_FN,
    TOK_IDENTIFIER,
    TOK_COLON,
    TOK_ARROW,
    TOK_OPEN_BRACE,
    TOK_CLOSE_BRACE,
    TOK_OPEN_PAREN,
    TOK_CLOSE_PAREN,
    TOK_COMMA,
    TOK_RETURN,
    TOK_PLUS,
    TOK_MINUS,
    TOK_SEMI,
    TOK_NUMBER,
    TOK_CONST,
    TOK_VAR,
    TOK_EQUAL,
    TOK_TYPE,
    TOK_IF,
    TOK_ELSE,
    TOK_EQUAL_EQUAL,
    TOK_NOT_EQUAL,
    TOK_LESS,
    TOK_LESS_EQUAL,
    TOK_GREATER,
    TOK_GREATER_EQUAL,
    TOK_TRUE,
    TOK_FALSE,
    TOK_OPEN_BRACKET,
    TOK_CLOSE_BRACKET,
    TOK_STRING_LITERAL,
};

// Token structure
struct Token {
    TokenType type;
    std::string lexeme;
    int line;

    Token(TokenType type, std::string lexeme, int line)
        : type(type), lexeme(std::move(lexeme)), line(line) {}
};

// Lexer class
class Lexer {
private:
    std::string source;
    std::vector<Token> tokens;
    int current = 0;
    int line = 1;

    bool isAtEnd() const {
        return current >= source.length();
    }

    char advance() {
        return source[current++];
    }

    char peek() const {
        if (isAtEnd()) return '\0';
        return source[current];
    }

    char peekNext() const {
        if (current + 1 >= source.length()) return '\0';
        return source[current + 1];
    }

    bool match(char expected) {
        if (isAtEnd() || source[current] != expected) return false;
        current++;
        return true;
    }

    void skipWhitespace() {
        while (true) {
            char c = peek();
            switch (c) {
                case ' ':
                case '\r':
                case '\t':
                    advance();
                    break;
                case '\n':
                    line++;
                    advance();
                    break;
                case '/':
                    if (peekNext() == '/') {
                        // Comment until end of line
                        while (peek() != '\n' && !isAtEnd()) advance();
                    } else {
                        return;
                    }
                    break;
                default:
                    return;
            }
        }
    }

    bool isDigit(char c) const {
        return c >= '0' && c <= '9';
    }

    bool isAlpha(char c) const {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    bool isAlphaNumeric(char c) const {
        return isAlpha(c) || isDigit(c);
    }

    void addToken(TokenType type) {
        addToken(type, "");
    }

    void addToken(TokenType type, const std::string& lexeme) {
        tokens.emplace_back(type, lexeme, line);
    }

    void identifier() {
        int start = current - 1; // Start position (we already consumed the first character)
        while (isAlphaNumeric(peek())) advance();

        std::string text = source.substr(start, current - start);

        // Check for keywords
        if (text == "fn") {
            addToken(TOK_FN, text);
        } else if (text == "return") {
            addToken(TOK_RETURN, text);
        } else if (text == "const") {
            addToken(TOK_CONST, text);
        } else if (text == "var") {
            addToken(TOK_VAR, text);
        } else if (text == "if") {
            addToken(TOK_IF, text);
        } else if (text == "else") {
            addToken(TOK_ELSE, text);
        } else if (text == "true") {
            addToken(TOK_TRUE, text);
        } else if (text == "false") {
            addToken(TOK_FALSE, text);
        } else if (text == "u8" || text == "u16" || text == "u32" || text == "i8" || text == "i16" || text == "i32" || text == "bool" || text == "str") {
            addToken(TOK_TYPE, text);
        } else {
            addToken(TOK_IDENTIFIER, text);
        }
    }

    void number() {
        int start = current - 1; // Start position (we already consumed the first digit)
        while (isDigit(peek())) advance();

        std::string num = source.substr(start, current - start);
        addToken(TOK_NUMBER, num);
    }

    void negativeNumber() {
        int start = current - 1; // Start position (we already consumed the minus)
        while (isDigit(peek())) advance();

        std::string num = source.substr(start, current - start);
        addToken(TOK_NUMBER, num);
    }

    void stringLiteral() {
        int start = current; // Start after the opening quote
        
        while (peek() != '"' && !isAtEnd()) {
            if (peek() == '\n') line++;
            advance();
        }

        if (isAtEnd()) {
            throw std::runtime_error("Unterminated string at line " + std::to_string(line));
        }

        // The closing "
        advance();

        // Trim the surrounding quotes
        std::string value = source.substr(start, current - start - 1);
        addToken(TOK_STRING_LITERAL, value);
    }

public:
    explicit Lexer(std::string source) : source(std::move(source)) {}

    std::vector<Token> scanTokens() {
        while (!isAtEnd()) {
            skipWhitespace();
            if (isAtEnd()) break;

            char c = advance();

            switch (c) {
                case '(': addToken(TOK_OPEN_PAREN, "("); break;
                case ')': addToken(TOK_CLOSE_PAREN, ")"); break;
                case '{': addToken(TOK_OPEN_BRACE, "{"); break;
                case '}': addToken(TOK_CLOSE_BRACE, "}"); break;
                case '[': addToken(TOK_OPEN_BRACKET, "["); break;
                case ']': addToken(TOK_CLOSE_BRACKET, "]"); break;
                case ',': addToken(TOK_COMMA, ","); break;
                case ';': addToken(TOK_SEMI, ";"); break;
                case ':': addToken(TOK_COLON, ":"); break;
                case '+': addToken(TOK_PLUS, "+"); break;
                case '"': stringLiteral(); break;
                
                case '=':
                    if (match('=')) {
                        addToken(TOK_EQUAL_EQUAL, "==");
                    } else {
                        addToken(TOK_EQUAL, "=");
                    }
                    break;
                
                case '!':
                    if (match('=')) {
                        addToken(TOK_NOT_EQUAL, "!=");
                    } else {
                        std::cerr << "Unexpected character at line " << line << ": " << c << std::endl;
                    }
                    break;
                
                case '<':
                    if (match('=')) {
                        addToken(TOK_LESS_EQUAL, "<=");
                    } else {
                        addToken(TOK_LESS, "<");
                    }
                    break;
                
                case '>':
                    if (match('=')) {
                        addToken(TOK_GREATER_EQUAL, ">=");
                    } else {
                        addToken(TOK_GREATER, ">");
                    }
                    break;

                case '-':
                    if (match('>')) {
                        addToken(TOK_ARROW, "->");
                    } else if (isDigit(peek())) {
                        // Handle negative number
                        negativeNumber();
                    } else {
                        addToken(TOK_MINUS, "-");
                    }
                    break;

                default:
                    if (isDigit(c)) {
                        number();
                    } else if (isAlpha(c)) {
                        identifier();
                    } else {
                        std::cerr << "Unexpected character at line " << line << ": " << c << std::endl;
                    }
                    break;
            }
        }

        tokens.emplace_back(TOK_EOF, "", line);
        return tokens;
    }
};

// AST node types
class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual llvm::Value* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) = 0;
};

class NumberExprAST : public ExprAST {
    int64_t Val;
public:
    NumberExprAST(int64_t Val) : Val(Val) {}
    llvm::Value* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) override;
};

class BooleanExprAST : public ExprAST {
    bool Val;
public:
    BooleanExprAST(bool Val) : Val(Val) {}
    llvm::Value* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) override;
};

class StringLiteralExprAST : public ExprAST {
    std::string Val;
public:
    StringLiteralExprAST(std::string Val) : Val(std::move(Val)) {}
    llvm::Value* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) override;
};

class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST(std::string Name) : Name(std::move(Name)) {}
    llvm::Value* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) override;
};

class BinaryExprAST : public ExprAST {
    std::string Op;
    std::unique_ptr<ExprAST> LHS, RHS;
public:
    BinaryExprAST(std::string Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(std::move(Op)), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    llvm::Value* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) override;
};

class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;
public:
    CallExprAST(std::string Callee, std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(std::move(Callee)), Args(std::move(Args)) {}
    llvm::Value* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) override;
};

class ReturnExprAST : public ExprAST {
    std::unique_ptr<ExprAST> RetVal;
public:
    ReturnExprAST(std::unique_ptr<ExprAST> RetVal) : RetVal(std::move(RetVal)) {}
    llvm::Value* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) override;
};

class VarDeclAST : public ExprAST {
    std::string Name;
    std::string Type;
    bool IsConst;
    std::unique_ptr<ExprAST> Init;
public:
    VarDeclAST(std::string Name, std::string Type, bool IsConst, std::unique_ptr<ExprAST> Init)
        : Name(std::move(Name)), Type(std::move(Type)), IsConst(IsConst), Init(std::move(Init)) {}
    llvm::Value* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) override;
};

class IfExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Condition;
    std::vector<std::unique_ptr<ExprAST>> ThenBody;
    std::vector<std::unique_ptr<ExprAST>> ElseBody;
public:
    IfExprAST(std::unique_ptr<ExprAST> Condition, 
              std::vector<std::unique_ptr<ExprAST>> ThenBody,
              std::vector<std::unique_ptr<ExprAST>> ElseBody)
        : Condition(std::move(Condition)), ThenBody(std::move(ThenBody)), ElseBody(std::move(ElseBody)) {}
    llvm::Value* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) override;
};

class FunctionAST {
public:
    std::string Name;
    std::vector<std::pair<std::string, std::string>> Args; // (name, type)
    std::string ReturnType;
    std::vector<std::unique_ptr<ExprAST>> Body;

    FunctionAST(std::string Name, std::vector<std::pair<std::string, std::string>> Args,
                std::string ReturnType, std::vector<std::unique_ptr<ExprAST>> Body)
        : Name(std::move(Name)), Args(std::move(Args)), ReturnType(std::move(ReturnType)), Body(std::move(Body)) {}

    llvm::Function* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues);
};

// Parser class
class Parser {
private:
    std::vector<Token> tokens;
    int current = 0;

    Token peek() const {
        return tokens[current];
    }

    Token previous() const {
        return tokens[current - 1];
    }

    bool isAtEnd() const {
        return peek().type == TOK_EOF;
    }

    Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    bool check(TokenType type) const {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    bool match(TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    void consume(TokenType type, const std::string& message) {
        if (check(type)) {
            advance();
            return;
        }

        throw std::runtime_error(message);
    }

    std::unique_ptr<ExprAST> parsePrimary() {
        if (match(TOK_NUMBER)) {
            return std::make_unique<NumberExprAST>(std::stoll(previous().lexeme));
        } else if (match(TOK_TRUE)) {
            return std::make_unique<BooleanExprAST>(true);
        } else if (match(TOK_FALSE)) {
            return std::make_unique<BooleanExprAST>(false);
        } else if (match(TOK_STRING_LITERAL)) {
            return std::make_unique<StringLiteralExprAST>(previous().lexeme);
        } else if (match(TOK_OPEN_PAREN)) {
            auto expr = parseExpression();
            consume(TOK_CLOSE_PAREN, "Expected ')' after expression");
            return expr;
        } else if (match(TOK_IDENTIFIER)) {
            std::string name = previous().lexeme;

            if (match(TOK_OPEN_PAREN)) {
                // This is a function call
                std::vector<std::unique_ptr<ExprAST>> args;

                if (!check(TOK_CLOSE_PAREN)) {
                    do {
                        args.push_back(parseComparison());
                    } while (match(TOK_COMMA));
                }

                consume(TOK_CLOSE_PAREN, "Expected ')' after function arguments");

                return std::make_unique<CallExprAST>(name, std::move(args));
            }

            return std::make_unique<VariableExprAST>(name);
        }

        throw std::runtime_error("Expected primary expression");
    }

    std::string parseType() {
        if (match(TOK_OPEN_BRACKET)) {
            consume(TOK_CLOSE_BRACKET, "Expected ']' after '['");
            std::string elementType = parseType();
            return "[]" + elementType;
        } else if (match(TOK_TYPE)) {
            return previous().lexeme;
        } else {
            throw std::runtime_error("Expected type");
        }
    }

    std::unique_ptr<ExprAST> parseExpression() {
        if (match(TOK_RETURN)) {
            auto expr = parseComparison();
            consume(TOK_SEMI, "Expected ';' after return statement");
            return std::make_unique<ReturnExprAST>(std::move(expr));
        } else if (match(TOK_CONST) || match(TOK_VAR)) {
            bool isConst = previous().type == TOK_CONST;
            consume(TOK_IDENTIFIER, "Expected variable name");
            std::string name = previous().lexeme;

            // Optional type annotation
            std::string type = "u8"; // Default type
            if (match(TOK_COLON)) {
                type = parseType();
            }

            std::unique_ptr<ExprAST> init = nullptr;
            if (match(TOK_EQUAL)) {
                init = parseComparison();
            }
            consume(TOK_SEMI, "Expected ';' after variable declaration");

            return std::make_unique<VarDeclAST>(name, type, isConst, std::move(init));
        } else if (match(TOK_IF)) {
            consume(TOK_OPEN_PAREN, "Expected '(' after 'if'");
            auto condition = parseComparison();
            consume(TOK_CLOSE_PAREN, "Expected ')' after if condition");
            
            consume(TOK_OPEN_BRACE, "Expected '{' after if condition");
            std::vector<std::unique_ptr<ExprAST>> thenBody;
            while (!check(TOK_CLOSE_BRACE) && !isAtEnd()) {
                thenBody.push_back(parseExpression());
            }
            consume(TOK_CLOSE_BRACE, "Expected '}' after if body");
            
            std::vector<std::unique_ptr<ExprAST>> elseBody;
            if (match(TOK_ELSE)) {
                consume(TOK_OPEN_BRACE, "Expected '{' after 'else'");
                while (!check(TOK_CLOSE_BRACE) && !isAtEnd()) {
                    elseBody.push_back(parseExpression());
                }
                consume(TOK_CLOSE_BRACE, "Expected '}' after else body");
            }
            
            return std::make_unique<IfExprAST>(std::move(condition), std::move(thenBody), std::move(elseBody));
        }

        return parseComparison();
    }

    std::unique_ptr<ExprAST> parseComparison() {
        auto LHS = parseAddition();

        if (match(TOK_EQUAL_EQUAL)) {
            auto RHS = parseAddition();
            return std::make_unique<BinaryExprAST>("==", std::move(LHS), std::move(RHS));
        } else if (match(TOK_NOT_EQUAL)) {
            auto RHS = parseAddition();
            return std::make_unique<BinaryExprAST>("!=", std::move(LHS), std::move(RHS));
        } else if (match(TOK_LESS)) {
            auto RHS = parseAddition();
            return std::make_unique<BinaryExprAST>("<", std::move(LHS), std::move(RHS));
        } else if (match(TOK_LESS_EQUAL)) {
            auto RHS = parseAddition();
            return std::make_unique<BinaryExprAST>("<=", std::move(LHS), std::move(RHS));
        } else if (match(TOK_GREATER)) {
            auto RHS = parseAddition();
            return std::make_unique<BinaryExprAST>(">", std::move(LHS), std::move(RHS));
        } else if (match(TOK_GREATER_EQUAL)) {
            auto RHS = parseAddition();
            return std::make_unique<BinaryExprAST>(">=", std::move(LHS), std::move(RHS));
        }

        return LHS;
    }

    std::unique_ptr<ExprAST> parseAddition() {
        auto LHS = parsePrimary();

        if (match(TOK_PLUS)) {
            auto RHS = parsePrimary();
            return std::make_unique<BinaryExprAST>("+", std::move(LHS), std::move(RHS));
        }

        return LHS;
    }

    std::unique_ptr<FunctionAST> parseFunction() {
        consume(TOK_FN, "Expected 'fn' keyword");
        consume(TOK_IDENTIFIER, "Expected function name");
        std::string name = previous().lexeme;

        consume(TOK_OPEN_PAREN, "Expected '(' after function name");

        std::vector<std::pair<std::string, std::string>> args;
        if (!check(TOK_CLOSE_PAREN)) {
            do {
                consume(TOK_IDENTIFIER, "Expected parameter name");
                std::string paramName = previous().lexeme;

                consume(TOK_COLON, "Expected ':' after parameter name");
                std::string paramType = parseType();

                args.emplace_back(paramName, paramType);
            } while (match(TOK_COMMA));
        }

        consume(TOK_CLOSE_PAREN, "Expected ')' after parameters");

        // Parse the return type
        std::string returnType;
        if (match(TOK_ARROW)) {
            returnType = parseType();
        }

        consume(TOK_OPEN_BRACE, "Expected '{' before function body");

        std::vector<std::unique_ptr<ExprAST>> body;
        while (!check(TOK_CLOSE_BRACE) && !isAtEnd()) {
            body.push_back(parseExpression());
        }

        consume(TOK_CLOSE_BRACE, "Expected '}' after function body");

        return std::make_unique<FunctionAST>(name, std::move(args), returnType, std::move(body));
    }

public:
    explicit Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

    std::vector<std::unique_ptr<FunctionAST>> parse() {
        std::vector<std::unique_ptr<FunctionAST>> functions;

        while (!isAtEnd()) {
            functions.push_back(parseFunction());
        }

        return functions;
    }
};

// Helper function to get LLVM type from type string
llvm::Type* getTypeFromString(const std::string& typeStr, llvm::LLVMContext& context) {
    if (typeStr == "u8" || typeStr == "i8") {
        return llvm::Type::getInt8Ty(context);
    } else if (typeStr == "u16" || typeStr == "i16") {
        return llvm::Type::getInt16Ty(context);
    } else if (typeStr == "u32" || typeStr == "i32") {
        return llvm::Type::getInt32Ty(context);
    } else if (typeStr == "bool") {
        return llvm::Type::getInt1Ty(context);
    } else if (typeStr == "str") {
        // String slice: struct { ptr: *u8, len: usize }
        llvm::Type* i8PtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
        llvm::Type* usizeType = llvm::Type::getInt64Ty(context); // usize as i64
        return llvm::StructType::get(context, {i8PtrType, usizeType});
    } else if (typeStr.substr(0, 2) == "[]") {
        // Slice type: []T -> struct { ptr: *T, len: usize }
        std::string elementType = typeStr.substr(2); // Remove "[]"
        llvm::Type* elemType = getTypeFromString(elementType, context);
        llvm::Type* elemPtrType = llvm::PointerType::get(elemType, 0);
        llvm::Type* usizeType = llvm::Type::getInt64Ty(context); // usize as i64
        return llvm::StructType::get(context, {elemPtrType, usizeType});
    }
    throw std::runtime_error("Unknown type: " + typeStr);
}

// Code generation implementations
llvm::Value* NumberExprAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    // Choose appropriate type based on value range
    llvm::Type* IntType;
    if (Val >= 0 && Val <= 255) {
        IntType = llvm::Type::getInt8Ty(TheModule->getContext());
    } else if (Val >= -128 && Val < 0) {
        IntType = llvm::Type::getInt8Ty(TheModule->getContext());
    } else if (Val >= 0 && Val <= 65535) {
        IntType = llvm::Type::getInt16Ty(TheModule->getContext());
    } else if (Val >= -32768 && Val < 0) {
        IntType = llvm::Type::getInt16Ty(TheModule->getContext());
    } else if (Val >= 0 && Val <= 4294967295ULL) {
        IntType = llvm::Type::getInt32Ty(TheModule->getContext());
    } else if (Val >= -2147483648LL && Val < 0) {
        IntType = llvm::Type::getInt32Ty(TheModule->getContext());
    } else {
        IntType = llvm::Type::getInt64Ty(TheModule->getContext());
    }
    return llvm::ConstantInt::get(IntType, Val, true);
}

llvm::Value* BooleanExprAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(TheModule->getContext()), Val ? 1 : 0);
}

llvm::Value* StringLiteralExprAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    // Create a global string constant
    llvm::Constant* StrConstant = llvm::ConstantDataArray::getString(TheModule->getContext(), Val, false);
    llvm::GlobalVariable* StrGlobal = new llvm::GlobalVariable(
        *TheModule,
        StrConstant->getType(),
        true, // isConstant
        llvm::GlobalValue::PrivateLinkage,
        StrConstant,
        "str"
    );
    
    // Create a string slice struct { ptr: *u8, len: usize }
    llvm::Type* i8PtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(TheModule->getContext()), 0);
    llvm::Type* usizeType = llvm::Type::getInt64Ty(TheModule->getContext());
    llvm::Type* sliceType = llvm::StructType::get(TheModule->getContext(), {i8PtrType, usizeType});
    
    // Get pointer to the string data
    llvm::Value* StrPtr = Builder.CreateBitCast(StrGlobal, i8PtrType);
    
    // Create the slice struct
    llvm::Value* SliceStruct = llvm::UndefValue::get(sliceType);
    SliceStruct = Builder.CreateInsertValue(SliceStruct, StrPtr, 0); // ptr
    SliceStruct = Builder.CreateInsertValue(SliceStruct, llvm::ConstantInt::get(usizeType, Val.length()), 1); // len
    
    return SliceStruct;
}

llvm::Value* VariableExprAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    llvm::Value* V = NamedValues[Name];
    if (!V)
        throw std::runtime_error("Unknown variable name: " + Name);
    
    // Get the type from the allocated value (for newer LLVM versions)
    llvm::AllocaInst* Alloca = llvm::cast<llvm::AllocaInst>(V);
    llvm::Type* LoadType = Alloca->getAllocatedType();
    return Builder.CreateLoad(LoadType, V, Name.c_str());
}

llvm::Value* BinaryExprAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    llvm::Value* L = LHS->codegen(Builder, TheModule, NamedValues);
    llvm::Value* R = RHS->codegen(Builder, TheModule, NamedValues);

    if (!L || !R)
        return nullptr;

    if (Op == "+")
        return Builder.CreateAdd(L, R, "addtmp");
    else if (Op == "==")
        return Builder.CreateICmpEQ(L, R, "cmptmp");
    else if (Op == "!=")
        return Builder.CreateICmpNE(L, R, "cmptmp");
    else if (Op == "<")
        return Builder.CreateICmpULT(L, R, "cmptmp");
    else if (Op == "<=")
        return Builder.CreateICmpULE(L, R, "cmptmp");
    else if (Op == ">")
        return Builder.CreateICmpUGT(L, R, "cmptmp");
    else if (Op == ">=")
        return Builder.CreateICmpUGE(L, R, "cmptmp");

    throw std::runtime_error("Invalid binary operator: " + Op);
}

llvm::Value* CallExprAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    llvm::Function* CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
        throw std::runtime_error("Unknown function referenced: " + Callee);

    if (CalleeF->arg_size() != Args.size())
        throw std::runtime_error("Incorrect number of arguments passed");

    std::vector<llvm::Value*> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->codegen(Builder, TheModule, NamedValues));
        if (!ArgsV.back())
            return nullptr;
    }

    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

llvm::Value* ReturnExprAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    llvm::Value* RetVal = this->RetVal->codegen(Builder, TheModule, NamedValues);
    if (!RetVal)
        return nullptr;

    Builder.CreateRet(RetVal);
    return RetVal;
}

llvm::Value* VarDeclAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    llvm::Type* VarType = getTypeFromString(Type, TheModule->getContext());
    llvm::AllocaInst* Alloca = Builder.CreateAlloca(VarType, nullptr, Name);
    
    if (Init) {
        llvm::Value* InitVal = Init->codegen(Builder, TheModule, NamedValues);
        if (!InitVal)
            return nullptr;
        Builder.CreateStore(InitVal, Alloca);
    } else {
        // Initialize with zero/null value
        llvm::Value* ZeroVal = llvm::Constant::getNullValue(VarType);
        Builder.CreateStore(ZeroVal, Alloca);
    }

    NamedValues[Name] = Alloca;
    return Alloca;
}

llvm::Value* IfExprAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    llvm::Value* CondV = Condition->codegen(Builder, TheModule, NamedValues);
    if (!CondV)
        return nullptr;

    // Convert condition to a bool by comparing non-equal to 0
    CondV = Builder.CreateICmpNE(CondV, llvm::ConstantInt::get(CondV->getType(), 0), "ifcond");

    llvm::Function* TheFunction = Builder.GetInsertBlock()->getParent();

    // Create blocks for the then and else cases. Insert the 'then' block at the end of the function.
    llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(TheModule->getContext(), "then", TheFunction);
    llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(TheModule->getContext(), "else", TheFunction);
    llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(TheModule->getContext(), "ifcont", TheFunction);

    Builder.CreateCondBr(CondV, ThenBB, ElseBB);

    // Emit then value.
    Builder.SetInsertPoint(ThenBB);
    llvm::Value* ThenV = nullptr;
    for (auto& Expr : ThenBody) {
        ThenV = Expr->codegen(Builder, TheModule, NamedValues);
    }
    // Only create branch if the block doesn't already have a terminator (like return)
    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateBr(MergeBB);
    }
    // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
    ThenBB = Builder.GetInsertBlock();

    // Emit else block.
    Builder.SetInsertPoint(ElseBB);
    llvm::Value* ElseV = nullptr;
    for (auto& Expr : ElseBody) {
        ElseV = Expr->codegen(Builder, TheModule, NamedValues);
    }
    // Only create branch if the block doesn't already have a terminator (like return)
    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateBr(MergeBB);
    }
    // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
    ElseBB = Builder.GetInsertBlock();

    // Emit merge block.
    Builder.SetInsertPoint(MergeBB);

    // For now, if statements don't return values, so we return a dummy value
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(TheModule->getContext()), 0);
}

llvm::Function* FunctionAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    // Create function prototype
    std::vector<llvm::Type*> ArgTypes;
    for (const auto& arg : Args) {
        ArgTypes.push_back(getTypeFromString(arg.second, TheModule->getContext()));
    }

    llvm::Type* RetType = ReturnType.empty() ? 
        llvm::Type::getVoidTy(TheModule->getContext()) : 
        getTypeFromString(ReturnType, TheModule->getContext());

    llvm::FunctionType* FT = llvm::FunctionType::get(
        RetType,        // Return type
        ArgTypes,       // Arg types
        false          // Not vararg
    );

    llvm::Function* F = llvm::Function::Create(
        FT,
        llvm::Function::ExternalLinkage,
        Name,
        TheModule
    );

    // Set names for all arguments
    unsigned ArgIdx = 0;
    for (auto& Arg : F->args())
        Arg.setName(Args[ArgIdx++].first);

    // Create a new basic block to start insertion into
    llvm::BasicBlock* BB = llvm::BasicBlock::Create(TheModule->getContext(), "entry", F);
    Builder.SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map
    NamedValues.clear();
    unsigned Idx = 0;
    for (auto& Arg : F->args()) {
        // Create an alloca for this variable using the correct type
        llvm::Type* ArgType = getTypeFromString(Args[Idx].second, TheModule->getContext());
        llvm::AllocaInst* Alloca = Builder.CreateAlloca(ArgType, nullptr, Arg.getName());

        // Store the initial value into the alloca
        Builder.CreateStore(&Arg, Alloca);

        // Add arguments to variable symbol table
        NamedValues[std::string(Arg.getName())] = Alloca;
        Idx++;
    }

    // Generate code for each expression in the function body
    for (auto& Expr : Body) {
        Expr->codegen(Builder, TheModule, NamedValues);
    }

    // Validate the generated code, checking for consistency
    llvm::verifyFunction(*F);

    return F;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Initialize LLVM
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // Create a LLVM context and module
    llvm::LLVMContext Context;
    std::unique_ptr<llvm::Module> TheModule = std::make_unique<llvm::Module>("my cool compiler", Context);

    // Create a builder for IR generation
    llvm::IRBuilder<> Builder(Context);

    // Tokenize the source code
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();

    // Parse the tokens into an AST
    Parser parser(tokens);
    std::vector<std::unique_ptr<FunctionAST>> functions = parser.parse();

    // Create a map to store variable values
    std::map<std::string, llvm::Value*> NamedValues;

    // Generate code from the AST
    for (auto& function : functions) {
        function->codegen(Builder, TheModule.get(), NamedValues);
    }

    // Print out the generated LLVM IR
    std::string output;
    llvm::raw_string_ostream out(output);
    TheModule->print(out, nullptr);
    std::cout << output;

    // Now create the output binary
    std::string TargetTriple = llvm::sys::getDefaultTargetTriple();
    TheModule->setTargetTriple(TargetTriple);

    std::string Error;
    const llvm::Target* Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    if (!Target) {
        std::cerr << "Failed to get target: " << Error << std::endl;
        return 1;
    }

    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>();
    auto TargetMachine = Target->createTargetMachine(TargetTriple, "generic", "", opt, RM);

    TheModule->setDataLayout(TargetMachine->createDataLayout());

    std::string ObjectFilename = "output.o";
    std::error_code EC;
    llvm::raw_fd_ostream dest(ObjectFilename, EC, llvm::sys::fs::OF_None);

    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        return 1;
    }

    llvm::legacy::PassManager pass;
    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        std::cerr << "TargetMachine can't emit a file of this type" << std::endl;
        return 1;
    }

    pass.run(*TheModule);
    dest.close();

    // Finish up by creating an executable using system compiler
    std::string cmd = "clang " + ObjectFilename + " -o output";
    system(cmd.c_str());

    std::cout << "Compilation completed successfully." << std::endl;

    return 0;
}
