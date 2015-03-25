#pragma once
#include <memory>
#include <vector>
#include "file_handling.h"

enum class TokenType {
    //sigils
    OpenBracket,
    CloseBracket,
    OpenCurlyBracket,
    CloseCurlyBracket,
    Semicolon,
    Colon,
    ThinArrow,
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

    //identifiers
    Identifier,
    //"typed" tokens
    Number,
    String,
    //undecided - will be resolved in the next stage
    Undecided,
    //EOF
    Eof
};

typedef long double TokenNumber;
struct TokenValue {

    std::unique_ptr<std::string> ptr_s;
    std::unique_ptr<long double> ptr_num;


    TokenValue(std::string string) : ptr_s(new std::string(string)){};
    TokenValue(TokenNumber number) : ptr_num(new TokenNumber(number)){};
    TokenValue()  {};
    const operator bool const () {
        return ptr_s || ptr_num; 

    }
};

struct Token {
    TokenType type;
    TokenValue value;
    PositionRange pos;

    Token(TokenType type, PositionRange pos) : type(type), pos(pos) {};
    Token(TokenType type, std::string string,  PositionRange pos) : type(type), value(string), pos(pos) {};
    Token(TokenType type, TokenNumber number,  PositionRange pos) : type(type), value(number), pos(pos) {};
};

std::ostream& operator <<(std::ostream &out, const TokenType &token_type);
std::ostream& operator <<(std::ostream &out, const TokenValue &token_value);
std::ostream& operator <<(std::ostream &out, const Token &token);
std::vector<Token> tokenize_string(std::string s);
