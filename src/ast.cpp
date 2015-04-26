#include "ast.h"
#include <sstream>
#include "assert.h"

// -----------------------------------------------------
// PARSING
class Parser;
enum class Precedence
{
    Lowest = 0,
    Statement,

    // conditionals
    ConditionalTerm,
    ConditionalFactor,

    // mathy ops
    MathSum,
    MathProduct,

    // bracketed expressions
    Brackets,
    Highest,
};

class ParserCursor {
    std::vector<Token>& tokens;
    PositionIndex       index;
    Token               eof_token;

public:

    ParserCursor(std::vector<Token>& tokens, std::string& file_data) : tokens(
            tokens), index(0),
        eof_token(TokenType::Eof,
                  PositionRange(file_data.size(), file_data.size(),
                                file_data)) {}

    const Token& expect(TokenType type, std::string error_info = "") {
        const Token& t = this->get();

        if (t.type != type) {
            std::stringstream error;
            error << "expected token type: " << type << " | found: " << t <<
                " at " <<
                t.pos;
            error << error_info;
            throw std::runtime_error(error.str());
        }

        // assert(t.type == type);
        this->index++;
        return t;
    }

    const Token& advance() {
        const Token& t = this->get();

        this->index++;

        return t;
    }

    const Token& get() const {
        if (this->index >= this->tokens.size()) {
            return this->eof_token;
        }
        return this->tokens[this->index];
    }

    const PositionRange get_current_range() {
        return this->get().pos;
    }
};
class IParserPrefix {
public:

    virtual std::shared_ptr<IAST>parse(Parser& parser) = 0;
    virtual bool should_apply(const Token& t) const = 0;
};

class IParserInfix {
public:

    virtual std::shared_ptr<IAST>parse(Parser               & parser,
                                       std::shared_ptr<IAST>& left) = 0;
    virtual bool should_apply(const Token& t) const = 0;
    virtual Precedence get_precedence() const       = 0;
};


class Parser {
    std::vector<IParserPrefix *>prefix_parsers;
    std::vector<IParserInfix *>infix_parsers;

public:

    // cursor is visible to anyone who has access to parser.
    ParserCursor cursor;

    Parser(std::vector<Token>& tokens, std::string& file_data) : cursor(tokens,
                                                                        file_data)
    {}

    void add_prefix_parser(IParserPrefix *parser) {
        this->prefix_parsers.push_back(parser);
    }

    void add_infix_parser(IParserInfix *parser) {
        this->infix_parsers.push_back(parser);
    }

    std::shared_ptr<IAST>parse(Precedence min_precedence) {
        const Token& t_prefix = cursor.get();

        // look for the prefix parser corresponding to our token;
        IParserPrefix *prefix = nullptr;

        for (IParserPrefix *parser : prefix_parsers) {
            if (parser->should_apply(t_prefix)) {
                prefix = parser;
                break;
            }
        }

        if (prefix == nullptr) {
            std::stringstream error;
            error << "unable to find prefix parser for token: ";
            error << t_prefix.pos;
            throw std::runtime_error(error.str());
            assert(false
                   && "unable to find prefix parser");
        }

        // parse using the prefix parser
        std::shared_ptr<IAST>left_ast = prefix->parse(*this);

        while (true) {
            // get the new token at the cursor head
            const Token& t_infix = cursor.get();

            // find the infix parser corresponding to the head
            IParserInfix *infix = nullptr;

            for (IParserInfix *parser : infix_parsers) {
                if (parser->should_apply(t_infix)) {
                    infix = parser;
                    break;
                }
            }

            // if there is no corresponding infix operator, quit
            if (infix == nullptr) {
                break;
            }

            // if the precendence of our operator is lower than the _minimum_,
            // then
            // quit
            if (infix->get_precedence() < min_precedence) {
                break;
            }

            // otherwise, parse
            left_ast = infix->parse(*this, left_ast);
        }

        return left_ast;
    }
};


// -----------------------------------------------------
// CONCRETER PARSERS INSTANCES
class OperatorParserPrefix : public IParserPrefix {
    TokenType op_type;

public:

    OperatorParserPrefix(TokenType op_type)
        : op_type(op_type) {}

    virtual bool should_apply(const Token& t) const {
        return t.type == this->op_type;
    }

    std::shared_ptr<IAST>parse(Parser& parser) {
        const Token& op_t = parser.cursor.expect(this->op_type);

        // bind the prefix operators the tightest
        std::shared_ptr<IAST>inner    = parser.parse(Precedence::Highest);
        PositionRange         position = inner->position.extend_end(
            parser.cursor.get_current_range());


        std::shared_ptr<IAST>prefix_ast(new ASTPrefixExpr(op_t, inner,
                                                          position));

        return prefix_ast;
    }
};

class LiteralParserPrefix : public IParserPrefix {
public:

    bool should_apply(const Token& t) const {
        return
            t.type == TokenType::Identifier
            || t.type == TokenType::LiteralInt
            || t.type == TokenType::LiteralString
            || t.type == TokenType::LiteralFloat;
    }

    std::shared_ptr<IAST>parse(Parser& parser) {
        const Token& literal_token = parser.cursor.advance();
        IAST         *ast_ptr      = new ASTLiteral(literal_token,
                                                    literal_token.pos);

        return std::shared_ptr<IAST>(ast_ptr);
    }
};

class BracketsParserPrefix : public IParserPrefix {
    bool should_apply(const Token& t) const {
        return t.type == TokenType::OpenBracket;
    }

    std::shared_ptr<IAST>parse(Parser& parser) {
        const Token&          literal_token = parser.cursor.advance();
        std::shared_ptr<IAST>ast_ptr       = parser.parse(Precedence::Lowest);

        parser.cursor.expect(TokenType::CloseBracket, "expected close bracket");
        return ast_ptr;
    }
};

class BlockParserPrefix : public IParserPrefix {
    bool should_apply(const Token& t) const {
        return t.type == TokenType::OpenCurlyBracket;
    }

    std::shared_ptr<IAST>parse(Parser& parser) {
        const Token&                        open_bracket_token =
            parser.cursor.advance();

        PositionRange                       position = open_bracket_token.pos;

        std::vector<std::shared_ptr<IAST> >statements;

        while (parser.cursor.get().type != TokenType::CloseCurlyBracket) {
            std::shared_ptr<IAST>statement = parser.parse(Precedence::Lowest);
            statements.push_back(statement);
            position.end = statement->position.end;
        }

        parser.cursor.expect(TokenType::CloseCurlyBracket,
                             "expected } to close block");

        return std::shared_ptr<IAST>(new ASTBlock(statements, nullptr,
                                                  position));
    }
};

class FunctionDefinitionPrefix : public IParserPrefix {
    bool should_apply(const Token& t) const {
        return t.type == TokenType::Fn;
    }

    std::shared_ptr<IAST>parse(Parser& parser) {
        const Token&  fn_token = parser.cursor.advance();
        PositionRange position = fn_token.pos;

        const Token&  fn_name = parser.cursor.advance();

        if (fn_name.type != TokenType::Identifier) {
            std::stringstream error;
            error << "expected identifier after fn. found: " << fn_name <<
                " at " <<
                fn_name.pos;
            throw std::runtime_error(error.str());
        }

        parser.cursor.expect(TokenType::OpenBracket,
                             "expect ( after fn identifier");
        std::vector<ASTFunctionDefinition::Argument>args;

        while (true) {
            std::shared_ptr<IAST>name = parser.parse(Precedence::Lowest);
            parser.cursor.expect(TokenType::Colon,
                                 ": needed to separate <id> and <type>");
            std::shared_ptr<IAST>type = parser.parse(Precedence::Lowest);

            args.push_back(std::make_pair(name, type));

            if (parser.cursor.get().type == TokenType::CloseBracket) {
                break;
            }
            parser.cursor.expect(TokenType::Comma);
        }
        parser.cursor.expect(TokenType::CloseBracket);

        parser.cursor.expect(TokenType::ThinArrow);
        std::shared_ptr<IAST>return_type = parser.parse(Precedence::Lowest);

        std::shared_ptr<IAST>block = parser.parse(Precedence::Lowest);

        position.end = parser.cursor.get_current_range().end;

        return std::shared_ptr<IAST>(new ASTFunctionDefinition(fn_name,
                                                               args,
                                                               return_type,
                                                               block,
                                                               position));
    }
};

class VariableDefinitionPrefix : public IParserPrefix {
    bool should_apply(const Token& t) const {
        return t.type == TokenType::Let;
    }

    std::shared_ptr<IAST>parse(Parser& parser) {
        PositionRange position  = parser.cursor.get_current_range();
        const Token&  let_token = parser.cursor.expect(TokenType::Let);


        std::shared_ptr<IAST>name = parser.parse(Precedence::Lowest);


        std::shared_ptr<IAST>type = nullptr;

        // you need the if  for the typing rule to be optional
        // if (parser.cursor.get().type == TokenType::Colon) {
        parser.cursor.expect(TokenType::Colon,
                             "expected : after variable name");

        type = parser.parse(Precedence::Highest);

        // }

        std::shared_ptr<IAST>rhs_value = nullptr;

        if (parser.cursor.get().type == TokenType::Equals) {
            parser.cursor.expect(TokenType::Equals);
            rhs_value = parser.parse(Precedence::Highest);
        }


        position = position.extend_end(parser.cursor.get_current_range());

        return std::shared_ptr<ASTVariableDefinition>(new ASTVariableDefinition(
                                                          name,
                                                          type,
                                                          rhs_value,
                                                          position));
    }
};


class OperatorParserInfix : public IParserInfix {
    TokenType  op_type;
    Precedence precedence;

public:

    OperatorParserInfix(TokenType op_type, Precedence precedence) :
        op_type(op_type), precedence(precedence) {}

    bool should_apply(const Token& t) const {
        return t.type == this->op_type;
    }

    Precedence get_precedence() const {
        return this->precedence;
    }

    std::shared_ptr<IAST>parse(Parser& parser, std::shared_ptr<IAST>& left) {
        const Token&          op_t  = parser.cursor.expect(this->op_type);
        std::shared_ptr<IAST>right = parser.parse(this->precedence);

        PositionRange         position = left->position.extend_end(
            right->position);

        std::shared_ptr<IAST>ast(new ASTInfixExpr(left, op_t, right, position));

        return ast;
    }
};

class FunctionCallParserInfix : public IParserInfix {
public:

    bool should_apply(const Token& t) const {
        return t.type == TokenType::OpenBracket;
    }

    Precedence get_precedence() const {
        return Precedence::Highest;
    }

    std::shared_ptr<IAST>parse(Parser& parser, std::shared_ptr<IAST>& left) {
        parser.cursor.expect(TokenType::OpenBracket);


        std::vector<std::shared_ptr<IAST> >params;

        while (true) {
            if (parser.cursor.get().type == TokenType::CloseBracket) {
                break;
            }

            std::shared_ptr<IAST>param = parser.parse(Precedence::Lowest);
            params.push_back(param);

            if (parser.cursor.get().type == TokenType::Comma) {
                parser.cursor.advance();
                continue;
            }
        }
        parser.cursor.expect(TokenType::CloseBracket);


        return std::shared_ptr<IAST>(new ASTFunctionCall(left, params,
                                                         left->position.
                                                         extend_end(
                                                             parser.cursor.
                                                             get_current_range())));
    }
};

class StatementParserPostfix : public IParserInfix {
public:

    bool should_apply(const Token& t) const {
        return t.type == TokenType::Semicolon;
    }

    Precedence get_precedence() const {
        return Precedence::Statement;
    }

    std::shared_ptr<IAST>parse(Parser& parser, std::shared_ptr<IAST>& left) {
        const Token&  t = parser.cursor.expect(TokenType::Semicolon);

        PositionRange position = left->position.extend_end(
            parser.cursor.get_current_range());


        return std::shared_ptr<IAST>(new ASTStatement(left, position));
    }
};

// -----------------------------------------------------
// CORE PARSING FUNCTION
std::shared_ptr<IAST>parse(std::vector<Token>& tokens, std::string& file_data) {
    Parser p(tokens, file_data);

    p.add_prefix_parser(new LiteralParserPrefix);
    p.add_prefix_parser(new BracketsParserPrefix);
    p.add_prefix_parser(new BlockParserPrefix);
    p.add_prefix_parser(new FunctionDefinitionPrefix);
    p.add_prefix_parser(new VariableDefinitionPrefix);
    p.add_prefix_parser(new OperatorParserPrefix(TokenType::Minus));
    p.add_prefix_parser(new OperatorParserPrefix(TokenType::CondNot));

    // conditional factors
    p.add_infix_parser(new OperatorParserInfix(TokenType::CondL,
                                               Precedence::ConditionalFactor));
    p.add_infix_parser(new OperatorParserInfix(TokenType::CondG,
                                               Precedence::ConditionalFactor));
    p.add_infix_parser(new OperatorParserInfix(TokenType::CondGEQ,
                                               Precedence::ConditionalFactor));
    p.add_infix_parser(new OperatorParserInfix(TokenType::CondLEQ,
                                               Precedence::ConditionalFactor));
    p.add_infix_parser(new OperatorParserInfix(TokenType::CondEQ,
                                               Precedence::ConditionalFactor));
    p.add_infix_parser(new OperatorParserInfix(TokenType::CondNEQ,
                                               Precedence::ConditionalFactor));

    // function call
    p.add_infix_parser(new FunctionCallParserInfix);

    //
    // conditional terms
    p.add_infix_parser(new OperatorParserInfix(TokenType::CondAnd,
                                               Precedence::ConditionalTerm));
    p.add_infix_parser(new OperatorParserInfix(TokenType::CondOr,
                                               Precedence::ConditionalTerm));

    // math operators
    p.add_infix_parser(new OperatorParserInfix(TokenType::Plus,
                                               Precedence::MathSum));
    p.add_infix_parser(new OperatorParserInfix(TokenType::Minus,
                                               Precedence::MathSum));
    p.add_infix_parser(new OperatorParserInfix(TokenType::Multiply,
                                               Precedence::MathProduct));
    p.add_infix_parser(new OperatorParserInfix(TokenType::Divide,
                                               Precedence::MathProduct));

    // statement (postfix)
    p.add_infix_parser(new StatementParserPostfix);

    std::vector<std::shared_ptr<IAST> >children;

    while (p.cursor.get().type != TokenType::Eof) {
        std::shared_ptr<IAST>ast = p.parse(Precedence::Lowest);
        children.push_back(ast);
    }

    PositionRange total_range = PositionRange(0, file_data.size(), file_data);
    return std::shared_ptr<IAST>(new ASTRoot(children, total_range));
}

// -----------------------------------------------------
// VISITOR IMPL
void IASTVisitor::inspect_root(ASTRoot& root) {
    root.traverse_inner(*this);
}

void IASTVisitor::inspect_literal(ASTLiteral& literal) {
    literal.traverse_inner(*this);
}

void IASTVisitor::inspect_block(ASTBlock& block) {
    block.traverse_inner(*this);
}

void IASTVisitor::inspect_infix_expr(ASTInfixExpr& infix) {
    infix.traverse_inner(*this);
}

void IASTVisitor::inspect_prefix_expr(ASTPrefixExpr& prefix) {
    prefix.traverse_inner(*this);
}

void IASTVisitor::inspect_statement(ASTStatement& statement) {
    statement.traverse_inner(*this);
}

void IASTVisitor::inspect_fn_definition(ASTFunctionDefinition& fn_defn) {
    fn_defn.traverse_inner(*this);
}

void IASTVisitor::inspect_fn_call(ASTFunctionCall& fn_call) {
    fn_call.traverse_inner(*this);
}

void IASTVisitor::inspect_variable_definition(
    ASTVariableDefinition& variable_defn)
{
    variable_defn.traverse_inner(*this);
}

void IASTGenericVisitor::inspect_root(ASTRoot& root) {
    this->inspect_ast(root);
}

void IASTGenericVisitor::inspect_literal(ASTLiteral& literal) {
    this->inspect_ast(literal);
}

void IASTGenericVisitor::inspect_block(ASTBlock& block) {
    this->inspect_ast(block);
}

void IASTGenericVisitor::inspect_infix_expr(ASTInfixExpr& infix) {
    this->inspect_ast(infix);
}

void IASTGenericVisitor::inspect_prefix_expr(ASTPrefixExpr& prefix) {
    this->inspect_ast(prefix);
}

void IASTGenericVisitor::inspect_statement(ASTStatement& statement) {
    this->inspect_ast(statement);
}

void IASTGenericVisitor::inspect_fn_definition(ASTFunctionDefinition& fn_defn) {
    this->inspect_ast(fn_defn);
}

void IASTGenericVisitor::inspect_fn_call(ASTFunctionCall& fn_call) {
    this->inspect_ast(fn_call);
}

void IASTGenericVisitor::inspect_variable_definition(
    ASTVariableDefinition& variable_defn) {
    this->inspect_ast(variable_defn);
}
