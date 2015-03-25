#pragma once
#include "ast.h"
#include <sstream>
#include <iostream>
#include <ostream>

class ASTPrettyPrinter : public IASTVisitor {
    private:
        int depth;
        std::ostream &out;

        void print_indent();
    public:
        ASTPrettyPrinter(std::ostream &out);
        void map_literal(ASTLiteral &literal);
        void map_block(ASTBlock &block);
        void map_infix_expr(ASTInfixExpr &infix);
        void map_prefix_expr(ASTPrefixExpr &prefix);
        void map_statement(ASTStatement &statement);
        void map_fn_definition(ASTFunctionDefinition &fn_defn);
};

void pretty_print_to_stream(IAST &ast, std::ostream &out) {
    ASTPrettyPrinter printer(out);
    ast.map(printer);
}

std::string pretty_print(IAST &ast) {
    std::stringstream sstream;
    pretty_print_to_stream(ast, sstream);
    return sstream.str();
}

ASTPrettyPrinter::ASTPrettyPrinter(std::ostream &out) : out(out), depth(0) {};

void ASTPrettyPrinter::print_indent() {
    for(int i = 0; i < this->depth; ++i) {
        out<<"  ";
    }
};

void ASTPrettyPrinter::map_literal(ASTLiteral &literal) {
    this->out<<literal.token;
};

void ASTPrettyPrinter::map_block(ASTBlock &block){
    out<<"\n";
    this->print_indent();
    out<<"{";
    this->depth++;

    for(auto statement: block.statements) {
        statement->map(*this);
    }

    this->depth--;
    out<<"\n";
    this->print_indent();
    out<<"}";
};

void ASTPrettyPrinter::map_infix_expr(ASTInfixExpr &infix){
    infix.left->map(*this);
    out<<" "<<infix.op<<" ";
    infix.right->map(*this);
};

void ASTPrettyPrinter::map_prefix_expr(ASTPrefixExpr &prefix){
    out<<prefix.op;
    prefix.expr->map(*this);
};

void ASTPrettyPrinter::map_statement(ASTStatement &statement){
    out<<"\n";
    this->print_indent();
    statement.inner->map(*this);
    out<<";";
};

void ASTPrettyPrinter::map_fn_definition(ASTFunctionDefinition &fn_defn){
    out<<"\n";
    this->print_indent();
    out<<"fn ";
    out<<fn_defn.fn_name;
    out<<"(";

    int commas_count = fn_defn.args.size() - 1;

    for (auto arg: fn_defn.args) {
        out<<arg.first;
        out<<" ";
        arg.second->map(*this);

        if(commas_count > 0) {
            out<<", ";
            commas_count--;
        }
    }


    out<<")"<<" -> ";
    fn_defn.return_type->map(*this);
    fn_defn.body->map(*this);

};

