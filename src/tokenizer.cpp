#include "tokenizer.h"
#include <iostream>
#include <string>
#include <map>
#include <assert.h>

std::ostream & operator <<(std::ostream& out, const TokenType& token_type){
    switch (token_type) {
    case TokenType::OpenBracket:
        out << "(";
        break;

    case TokenType::CloseBracket:
        out << ")";
        break;

    case TokenType::OpenCurlyBracket:
        out << "{";
        break;

    case TokenType::CloseCurlyBracket:
        out << "}";
        break;

    case TokenType::CondNot:
        out << "!";
        break;

    case TokenType::Comma:
        out << ",";
        break;

    case TokenType::Semicolon:
        out << ";";
        break;

    case TokenType::ThinArrow:
        out << "->";
        break;

    case TokenType::Colon:
        out << ":";
        break;

    case TokenType::Equals:
        out << "=";
        break;

    case TokenType::Plus:
        out << "+";
        break;

    case TokenType::Minus:
        out << "-";
        break;

    case TokenType::Multiply:
        out << "*";
        break;

    case TokenType::Divide:
        out << "/";
        break;

    case TokenType::CondEQ:
        out << "==";
        break;

    case TokenType::CondNEQ:
        out << "!=";
        break;

    case TokenType::CondGEQ:
        out << ">=";
        break;

    case TokenType::CondLEQ:
        out << "<=";
        break;

    case TokenType::CondL:
        out << "<";
        break;

    case TokenType::CondG:
        out << ">";
        break;

    case TokenType::CondAnd:
        out << "&&";
        break;

    case TokenType::CondOr:
        out << "||";
        break;

    case TokenType::If:
        out << "if";
        break;

    case TokenType::Else:
        out << "else";
        break;

    case TokenType::For:
        out << "for";
        break;

    case TokenType::Let:
        out << "let";
        break;

    case TokenType::Fn:
        out << "fn";
        break;

    case TokenType::Eof:
        out << "|EOF|";
        break;

    case TokenType::Identifier:
        out << "id-";
        break;

    case TokenType::Undecided:
        out << "un-";
        break;


    case TokenType::LiteralString:
    case TokenType::LiteralInt:
    case TokenType::LiteralFloat:
        break;
    }
    return out;
}


std::ostream& operator <<(std::ostream& out, const TokenValue& token_value){
    if (token_value.ptr_s) {
        out << *token_value.ptr_s;
    }else if (token_value.ptr_i) {
        out << *token_value.ptr_i;
    }else if (token_value.ptr_f) {
        out << *token_value.ptr_f;
    }

    return out;
}


std::ostream& operator <<(std::ostream& out, const Token& token){
    if (token.type == TokenType::LiteralString) {
        out << "\"" << token.value << "\"";
    }else {
        out << token.type << token.value;
    }
    return out;
}


bool is_alphabet(char c){
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z');
}


bool is_number(char c){
    return c >= '0' && c <= '9';
}


bool is_whitespace(char c){
    return c == ' ' || c == '\n' || c == '\t';
}


PositionIndex strip_whitespace(std::string s, PositionIndex i){
    while (is_whitespace(s[i])) {
        ++i;
    }

    return i;
}


const std::vector<std::pair<std::string, TokenType> > sigil_map =
{
    { "=",  TokenType::Equals            },
    { "->", TokenType::ThinArrow         },
    { "(",  TokenType::OpenBracket       },
    { ")",  TokenType::CloseBracket      },
    { ";",  TokenType::Semicolon         },
    { ",",  TokenType::Comma             },
    { ":",  TokenType::Colon             },
    { "{",  TokenType::OpenCurlyBracket  },
    { "}",  TokenType::CloseCurlyBracket },
    //math
    { "+",  TokenType::Plus              },
    { "-",  TokenType::Minus             },
    { "*",  TokenType::Multiply          },
    { "/",  TokenType::Divide            },
    //conditional factors
    { ">=", TokenType::CondGEQ           },
    { "<=", TokenType::CondLEQ           },
    { "==", TokenType::CondEQ            },
    { "!=", TokenType::CondNEQ           },
    { ">",  TokenType::CondG             },
    { "<",  TokenType::CondL             },
    { "!",  TokenType::CondNot           },
    //conditional terms
    { "&&", TokenType::CondAnd           },
    { "||", TokenType::CondOr            }
};

const std::map<std::string, TokenType> keywords_map =
{
    { "let",  TokenType::Let  },
    { "if",   TokenType::If   },
    { "else", TokenType::Else },
    { "for",  TokenType::For  },
    { "fn",   TokenType::Fn   },
};


std::vector<Token> tokenize_string(std::string& s){
    std::vector<Token> tokens;
    PositionIndex      i     = 0;
    PositionIndex      begin = 0;

    while (i < s.size()) {
        begin = i;
        for (auto it = sigil_map.begin(); it != sigil_map.end(); ++it) {
            std::string sigil_str        = it->first;
            TokenType   sigil_token_type = it->second;

            //if we find a sigil, split it out
            if (s.substr(i, sigil_str.size()) == sigil_str) {
                tokens.push_back(Token(sigil_token_type, PositionRange(begin, i, s)));
                i += sigil_str.size();

                //strip whitespace and get back to tokenization
                goto TOKENIZATION_END;
            }
        }
        //okay, no sigil was split so look for other kinds of tokens

        //comments
        if (s[i] == '#') {
            while (s[i] != '\n') {
                i++;
                if (i >= s.size()) {
                    break;
                }
            }
        }
        //strings
        else if (s[i] == '\"') {
            i++;
            std::string string;

            if (i >= s.size()) {
                assert(false && "unclosed string present");
            }
            while (s[i] != '\"') {
                string.push_back(s[i]);
                i++;
            }
            //skip over the closing quotes
            i++;
            tokens.push_back(Token(TokenType::LiteralString, string, PositionRange(begin, i, s)));
        }
        //numbers
        else if (is_number(s[i]) || (s[i] == '.')) {
            std::string number_string("");
            while (is_number(s[i]) || s[i] == '.') {
                number_string.push_back(s[i]);
                i++;
            }

            bool is_int = number_string.find('.') == std::string::npos;

            if (is_int) {
                long long num_i = std::stoll(number_string.c_str());
                tokens.push_back(Token(TokenType::LiteralInt, num_i, PositionRange(begin, i, s)));
            }else {
                long double num_f = std::stold(number_string.c_str());
                tokens.push_back(Token(TokenType::LiteralFloat, num_f, PositionRange(begin, i, s)));
            }
        }
        //identifiers
        else if (is_alphabet(s[i])) {
            std::string identifier_name("");
            while (is_alphabet(s[i]) || is_number(s[i]) || s[i] == '_') {
                identifier_name.push_back(s[i]);
                i++;
            }
            //check if it's a keyword. if it is, push a keyword. Otherwise, push an identifier
            for (auto it : keywords_map) {
                std::string name = it.first;
                if (name == identifier_name) {
                    TokenType identifier_token_type = it.second;
                    tokens.push_back(Token(identifier_token_type, PositionRange(begin, i, s)));
                    goto TOKENIZATION_END;
                }
            }
            //not a keyword, it's an identifier:
            tokens.push_back(Token(TokenType::Identifier, identifier_name, PositionRange(begin, i, s)));
        }
        //undecided strings
        else{
            std::string undecided_string("");
            while (!is_whitespace(s[i])) {
                undecided_string.push_back(s[i]);
                i++;
            }

            tokens.push_back(Token(TokenType::Undecided, undecided_string, PositionRange(begin, i, s)));
        }
 TOKENIZATION_END:
        i = strip_whitespace(s, i);
    }

    return tokens;
}
