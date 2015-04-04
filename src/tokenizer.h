#pragma once
#include <memory>
#include <vector>
#include "file_handling.h"
#include "assert.h"

enum class TokenType {
    //sigils
    OpenBracket,
    CloseBracket,
    OpenCurlyBracket,
    CloseCurlyBracket,
    Semicolon,
    Colon,
    Comma,
    ThinArrow,
    Equals,
    //math sigils
    Plus,
    Minus,
    Multiply,
    Divide,
    //conditional factors
    CondL,
    CondG,
    CondLEQ,
    CondGEQ,
    CondEQ,
    CondNEQ,
    CondNot, //unary not operator
    //conditional terms
    CondAnd,
    CondOr,

    //keywords
    If,
    Else,
    For,
    Let,
    Fn,
    //
    //identifiers
    Identifier,
    //"typed" tokens
    LiteralFloat,
    LiteralInt,
    LiteralString,
    //undecided - will be resolved in the next stage
    Undecided,
    //EOF
    Eof
};

struct TokenValue {
    std::unique_ptr<std::string> ptr_s;
    std::unique_ptr<long long> ptr_i;
    std::unique_ptr<long double> ptr_f;


    TokenValue(std::string string) : ptr_s(new std::string(string)){};
    TokenValue(long long i) : ptr_i(new long long(i)){};
    TokenValue(long double f) : ptr_f(new long double(f)){};
    TokenValue()  {};
    const operator bool const () {
        return ptr_s || ptr_i || ptr_f; 

    }
};

struct Token {
    TokenType type;
    TokenValue value;
    PositionRange pos;

    Token(TokenType type, PositionRange pos) : type(type), pos(pos) {};
    Token(TokenType type, std::string string,  PositionRange pos) : type(type), value(string), pos(pos) {};
    Token(TokenType type, long long i,  PositionRange pos) : type(type), value(i), pos(pos) {};
    Token(TokenType type, long double f,  PositionRange pos) : type(type), value(f), pos(pos) {};
};

std::ostream& operator <<(std::ostream &out, const TokenType &token_type);
std::ostream& operator <<(std::ostream &out, const TokenValue &token_value);
std::ostream& operator <<(std::ostream &out, const Token &token);
std::vector<Token> tokenize_string(std::string &file_data);
