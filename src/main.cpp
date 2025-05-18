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
#include <map>
#include <stdexcept>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/IR/LegacyPassManager.h"

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
    TOK_SEMI,
    TOK_NUMBER,
    TOK_CONST,
    TOK_VAR,
    TOK_EQUAL,
    TOK_TYPE,
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
        while (isAlphaNumeric(peek())) advance();

        std::string text = source.substr(current - (current - (current - source.find_last_not_of(" \t\r\n", current - 1) - 1)), current - source.find_last_not_of(" \t\r\n", current - 1) - 1);

        // Check for keywords
        if (text == "fn") {
            addToken(TOK_FN, text);
        } else if (text == "return") {
            addToken(TOK_RETURN, text);
        } else if (text == "const") {
            addToken(TOK_CONST, text);
        } else if (text == "var") {
            addToken(TOK_VAR, text);
        } else if (text == "u8") {
            addToken(TOK_TYPE, text);
        } else {
            addToken(TOK_IDENTIFIER, text);
        }
    }

    void number() {
        while (isDigit(peek())) advance();

        std::string num = source.substr(current - 1, 1);
        addToken(TOK_NUMBER, num);
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
                case ',': addToken(TOK_COMMA, ","); break;
                case ';': addToken(TOK_SEMI, ";"); break;
                case ':': addToken(TOK_COLON, ":"); break;
                case '+': addToken(TOK_PLUS, "+"); break;
                case '=': addToken(TOK_EQUAL, "="); break;

                case '-':
                    if (match('>')) {
                        addToken(TOK_ARROW, "->");
                    } else {
                        // Handle minus operator if needed
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
    int Val;
public:
    NumberExprAST(int Val) : Val(Val) {}
    llvm::Value* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) override;
};

class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST(std::string Name) : Name(std::move(Name)) {}
    llvm::Value* codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) override;
};

class BinaryExprAST : public ExprAST {
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;
public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
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
    bool IsConst;
    std::unique_ptr<ExprAST> Init;
public:
    VarDeclAST(std::string Name, bool IsConst, std::unique_ptr<ExprAST> Init)
        : Name(std::move(Name)), IsConst(IsConst), Init(std::move(Init)) {}
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

    std::unique_ptr<ExprAST> parseExpression() {
        if (match(TOK_NUMBER)) {
            return std::make_unique<NumberExprAST>(std::stoi(previous().lexeme));
        } else if (match(TOK_IDENTIFIER)) {
            std::string name = previous().lexeme;

            if (match(TOK_OPEN_PAREN)) {
                // This is a function call
                std::vector<std::unique_ptr<ExprAST>> args;

                if (!check(TOK_CLOSE_PAREN)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match(TOK_COMMA));
                }

                consume(TOK_CLOSE_PAREN, "Expected ')' after function arguments");

                return std::make_unique<CallExprAST>(name, std::move(args));
            }

            return std::make_unique<VariableExprAST>(name);
        } else if (match(TOK_RETURN)) {
            auto expr = parseExpression();
            consume(TOK_SEMI, "Expected ';' after return statement");
            return std::make_unique<ReturnExprAST>(std::move(expr));
        } else if (match(TOK_CONST) || match(TOK_VAR)) {
            bool isConst = previous().type == TOK_CONST;
            consume(TOK_IDENTIFIER, "Expected variable name");
            std::string name = previous().lexeme;

            consume(TOK_EQUAL, "Expected '=' after variable name");

            auto init = parseExpression();
            consume(TOK_SEMI, "Expected ';' after variable declaration");

            return std::make_unique<VarDeclAST>(name, isConst, std::move(init));
        }

        // Binary expressions
        auto LHS = parseExpression();

        if (match(TOK_PLUS)) {
            auto RHS = parseExpression();
            return std::make_unique<BinaryExprAST>('+', std::move(LHS), std::move(RHS));
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
                consume(TOK_TYPE, "Expected parameter type");
                std::string paramType = previous().lexeme;

                args.emplace_back(paramName, paramType);
            } while (match(TOK_COMMA));
        }

        consume(TOK_CLOSE_PAREN, "Expected ')' after parameters");

        // Parse the return type
        std::string returnType;
        if (match(TOK_ARROW)) {
            consume(TOK_TYPE, "Expected return type");
            returnType = previous().lexeme;
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

// Code generation implementations
llvm::Value* NumberExprAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(TheModule->getContext()), Val, true);
}

llvm::Value* VariableExprAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    llvm::Value* V = NamedValues[Name];
    if (!V)
        throw std::runtime_error("Unknown variable name: " + Name);
    return Builder.CreateLoad(llvm::Type::getInt8Ty(TheModule->getContext()), V, Name.c_str());
}

llvm::Value* BinaryExprAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    llvm::Value* L = LHS->codegen(Builder, TheModule, NamedValues);
    llvm::Value* R = RHS->codegen(Builder, TheModule, NamedValues);

    if (!L || !R)
        return nullptr;

    if (Op == '+')
        return Builder.CreateAdd(L, R, "addtmp");

    throw std::runtime_error("Invalid binary operator");
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
    llvm::Value* RetVal = RetVal->codegen(Builder, TheModule, NamedValues);
    if (!RetVal)
        return nullptr;

    Builder.CreateRet(RetVal);
    return RetVal;
}

llvm::Value* VarDeclAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    llvm::Value* InitVal = Init->codegen(Builder, TheModule, NamedValues);
    if (!InitVal)
        return nullptr;

    llvm::AllocaInst* Alloca = Builder.CreateAlloca(llvm::Type::getInt8Ty(TheModule->getContext()), nullptr, Name);
    Builder.CreateStore(InitVal, Alloca);

    NamedValues[Name] = Alloca;
    return Alloca;
}

llvm::Function* FunctionAST::codegen(llvm::IRBuilder<>& Builder, llvm::Module* TheModule, std::map<std::string, llvm::Value*>& NamedValues) {
    // Create function prototype
    std::vector<llvm::Type*> ArgTypes(Args.size(), llvm::Type::getInt8Ty(TheModule->getContext()));

    llvm::FunctionType* FT = llvm::FunctionType::get(
        llvm::Type::getInt8Ty(TheModule->getContext()),  // Return type (assuming u8)
        ArgTypes,                                        // Arg types
        false                                           // Not vararg
    );

    llvm::Function* F = llvm::Function::Create(
        FT,
        llvm::Function::ExternalLinkage,
        Name,
        TheModule
    );

    // Set names for all arguments
    unsigned Idx = 0;
    for (auto& Arg : F->args())
        Arg.setName(Args[Idx++].first);

    // Create a new basic block to start insertion into
    llvm::BasicBlock* BB = llvm::BasicBlock::Create(TheModule->getContext(), "entry", F);
    Builder.SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map
    NamedValues.clear();
    for (auto& Arg : F->args()) {
        // Create an alloca for this variable
        llvm::AllocaInst* Alloca = Builder.CreateAlloca(llvm::Type::getInt8Ty(TheModule->getContext()), nullptr, Arg.getName());

        // Store the initial value into the alloca
        Builder.CreateStore(&Arg, Alloca);

        // Add arguments to variable symbol table
        NamedValues[std::string(Arg.getName())] = Alloca;
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
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    TheModule->setTargetTriple(TargetTriple);

    std::string Error;
    const llvm::Target* Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    if (!Target) {
        std::cerr << "Failed to get target: " << Error << std::endl;
        return 1;
    }

    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
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
    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
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
