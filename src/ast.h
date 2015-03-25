#pragma once
#include "tokenizer.h"
#include "file_handling.h"
#include "type_system.h"

class IASTVisitor;
class ASTLiteral;
class ASTBlock;
class ASTInfixExpr;
class ASTPrefixExpr;
class ASTStatement;
class ASTFunctionDefinition;
class ASTVariableDefinition;

enum class ASTType {
    BracketedExpr,
    PrefixExpr,
    InfixExpr,
    Literal,
    Statement,
    Block,
    FunctionDefinition,
    VariableDefinition,
};

class IAST {
    protected:
        IAST(ASTType ast_type, PositionRange position) : ast_type(ast_type), position(position) {};
    public:
        ASTType ast_type;
        TypeSystemType type_system_type;
        PositionRange position;

        //calls the correct version of the visit on the visitor
        virtual void map(IASTVisitor &visitor) = 0;
        //recurses the traversal on sub elements
        virtual void traverse_inner(IASTVisitor &visitor) = 0;

};

//visitor
class IASTVisitor {
    public:
        virtual void map_literal(ASTLiteral &literal);
        virtual void map_block(ASTBlock &block);
        virtual void map_infix_expr(ASTInfixExpr &infix);
        virtual void map_prefix_expr(ASTPrefixExpr &prefix);
        virtual void map_statement(ASTStatement &statement);
        virtual void map_fn_definition(ASTFunctionDefinition &fn_defn);
        virtual void map_variable_definition(ASTVariableDefinition &variable_defn);
};



class ASTPrefixExpr : public IAST {
    public:
        const Token& op; 
        std::shared_ptr<IAST> expr;

        ASTPrefixExpr(const Token& op, std::shared_ptr<IAST> expr, PositionRange position) : 
            op(op), expr(expr), IAST(ASTType::PrefixExpr, position)  {};

        virtual void map(IASTVisitor &visitor) {
            visitor.map_prefix_expr(*this);
        };

        virtual void traverse_inner(IASTVisitor &visitor) {
            expr->map(visitor);
        };
};

class ASTInfixExpr : public IAST {
    public:
        const Token &op;
        std::shared_ptr<IAST> left;
        std::shared_ptr<IAST> right;

        ASTInfixExpr(std::shared_ptr<IAST> left, const Token &op, std::shared_ptr<IAST> right, PositionRange position) :
            left(left), op(op), right(right), IAST(ASTType::InfixExpr, position) {}

        virtual void map(IASTVisitor &visitor) {
            visitor.map_infix_expr(*this);
        };

        virtual void traverse_inner(IASTVisitor &visitor) {
            left->map(visitor);
            right->map(visitor);
        };
};

class ASTStatement : public IAST {
    public:
        std::shared_ptr<IAST> inner;

        ASTStatement(std::shared_ptr<IAST> inner, PositionRange position) :
            inner(inner), IAST(ASTType::Statement, position){}


        virtual void map(IASTVisitor &visitor) {
            visitor.map_statement(*this);
        };

        virtual void traverse_inner(IASTVisitor &visitor) {
            inner->map(visitor);
        };
};

class ASTBlock : public IAST {
    public:
        std::vector<std::shared_ptr<IAST>> statements;
        std::shared_ptr<IAST> return_expr;

        ASTBlock(std::vector<std::shared_ptr<IAST>> statements, std::shared_ptr<IAST> return_expr, PositionRange position) :
            statements(statements), return_expr(return_expr), IAST(ASTType::Block, position){}

        virtual void map(IASTVisitor &visitor) {
            visitor.map_block(*this);
        };

        virtual void traverse_inner(IASTVisitor &visitor) {
            for(auto statement: this->statements) {
                statement->map(visitor);
            }
        };
};

class ASTLiteral : public IAST {
    public:
        const Token &token;

        ASTLiteral(const Token &token, PositionRange position) : 
            token(token), IAST(ASTType::Literal, position)  {};

        virtual void map(IASTVisitor &visitor) {
            visitor.map_literal(*this);
        };

        virtual void traverse_inner(IASTVisitor &visitor) {};
};

class ASTFunctionDefinition : public IAST {
    public:
        typedef std::pair<const Token&, std::shared_ptr<IAST>> Argument;
        const Token  &fn_name;
        std::vector<Argument> args;

        std::shared_ptr<IAST> body;
        std::shared_ptr<IAST> return_type;

        ASTFunctionDefinition(const Token &fn_name, 
                std::vector<Argument> args, 
                std::shared_ptr<IAST> return_type,
                std::shared_ptr<IAST> body,
                PositionRange position) : 
            fn_name(fn_name), 
            args(args), 
            return_type(return_type), 
            body(body),
            IAST(ASTType::FunctionDefinition, position){
            }

        virtual void map(IASTVisitor &visitor) {
            visitor.map_fn_definition(*this);
        };

        virtual void traverse_inner(IASTVisitor &visitor) {
            this->body->map(visitor);
        };
};


class ASTVariableDefinition : public IAST {
    public:
        const Token &name;
        std::shared_ptr<IAST> type;
        std::shared_ptr<IAST> value;

        ASTVariableDefinition(const Token &name, std::shared_ptr<IAST> type, std::shared_ptr<IAST> value, PositionRange position) :
            name(name), type(type), value(value), IAST(ASTType::VariableDefinition, position) {};

        virtual void map(IASTVisitor &visitor) {
            visitor.map_variable_definition(*this);
        };

        virtual void traverse_inner(IASTVisitor &visitor) {
            this->type->map(visitor);

            if(this->value) {
                this->value->map(visitor);
            }
        };
};

std::shared_ptr<IAST> parse(std::vector<Token> &tokens);
